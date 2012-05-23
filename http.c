/*
	libhttpd - a C library to aid serving and responding to HTTP requests

	Copyright (C) 2009 onwards  Attie Grande (attie@attie.co.uk)

	This program is free software: you can redistribute it and/or modify it
	under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "http.h"
#include "session.h"
#include "buf.h"

#define HTTP_BLOCK_SIZE 128

void inline http_getField(unsigned char *start, unsigned char **end, unsigned char *endOfData, unsigned char delimiter) {
	while (*start != delimiter && start < endOfData) start++;
	if (*start == delimiter) {
		*start = '\0';
		*end = start - 1;
	} else {
		*end = NULL;
	}
}
void inline http_trimField(unsigned char **start, unsigned char **end) {
	if (start) while (**start == ' ' && *start < *end) (*start)++;
	if (end)   while (**end == ' '   && *end > *start) (*end)--;
}

hte add_header(struct http_data *data, unsigned char *field_name, unsigned char *field_value) {
	void *p;
	int c;
	
	c = data->headerc + 1;
	if ((p = realloc(data->headers, sizeof(*data->headers) * c)) == NULL) return HTE_NOMEM;
	data->headers = p;
	
	data->headers[data->headerc].name = field_name;
	data->headers[data->headerc].value = field_value;
	
	data->headerc = c;
	
	return HTE_NONE;
}

hte http_read(struct session_info *session) {
	hte ret = HTE_NONE;
	struct http_request *req;
	ssize_t rxLen;
	
	if (!session || !session->xfer.request) return HTE_INVALPARAM;
	req = session->xfer.request;
	
	req->state = STATE_START;
	
	while (req->state != STATE_COMPLETE) {
		if (req->buf == NULL) {
			if ((req->buf = buf_alloc(NULL, HTTP_BLOCK_SIZE)) == NULL) return HTE_NOMEM;
		} else {
			void *p;
			if ((p = buf_alloc(req->buf, req->buf->next + HTTP_BLOCK_SIZE)) == NULL) {
				buf_free(req->buf);
				return HTE_NOMEM;
			}
			req->buf = p;
		}
		
		if ((rxLen = recv(session->fd, &(req->buf->data[req->buf->next]), HTTP_BLOCK_SIZE, 0)) == -1) {
			ret = HTE_READ;
			goto die;
		}
		
		if (rxLen == 0) break;
		req->buf->next += rxLen;
		
		if ((ret = http_parse(session)) != HTE_NONE) goto die;
		
		if (req->state == STATE_ERROR) { ret = HTE_PARSE; goto die; }
	}
	
	if (req->state != STATE_COMPLETE) { ret = HTE_PARSE; goto die; }
	
	return HTE_NONE;
die:
	return ret;
}

hte http_parse(struct session_info *session) {
	hte ret;
	struct http_request *req;
	unsigned char *sol,  *eol;  /* {start|end} of line */
	unsigned char *sof1, *eof1; /* {start|end} of field 1 */
	unsigned char *sof2, *eof2; /* {start|end} of field 2 */
	unsigned char        *eod;  /* end of data */
	
	if (!session || !session->xfer.request) return HTE_INVALPARAM;
	req = session->xfer.request;
	
	sol = &(req->buf->data[req->parsePos]);
	eod = &(req->buf->data[req->buf->next - 1]);
	
	ret = HTE_NONE;
	
	while (sol <= eod && req->state != STATE_COMPLETE) {
		
		if (req->state == STATE_START ||
		    req->state == STATE_GETTING_HEADERS) {
			/* suck in a line --> sol <==> eol */
			for (eol = sol; *eol != '\r' && *eol != '\n' && eol <= eod; eol++);
			if (*eol != '\r' && *eol != '\n') { break; }
			*eol = '\0';
		}
		
		switch (req->state) {
			case STATE_START:
				sof1 = sol;
				
				/* get the method */
				http_getField(sof1, &eof1, eod, ' ');
				if (eof1 == NULL) { ret = HTE_PARSE; goto die; };
				req->method = sof1;
				sof1 = eof1 + 2;
				
				/* get the uri */
				http_getField(sof1, &eof1, eod, ' ');
				if (eof1 == NULL) { ret = HTE_PARSE; goto die; };
				req->uri = sof1;
				sof1 = eof1 + 2;
				
				/* get the http version */
				if (eof1 + 2 >= eol) { ret = HTE_PARSE; goto die; }
				req->httpVersion = eof1 + 2;
				
				req->state = STATE_GETTING_HEADERS;
				
				break;
				
			case STATE_GETTING_HEADERS:
				if (sol == eol) {
					if (req->data.contentLength > 0) {
						req->state = STATE_START_CONTENT;
					} else {
						req->state = STATE_COMPLETE;
					}
					break;
				}

				sof1 = sol;
				
				/* get the name */
				http_getField(sof1, &eof1, eod, ':'); if (eof1 == NULL) { ret = HTE_PARSE; goto die; };
				/* get the value */
				if (eof1 + 2 >= eol) { ret = HTE_PARSE; goto die; }; sof2 = eof1 + 2; eof2 = eol - 1;
				
				/* trim them */
				http_trimField(&sof1, &eof1); eof1[1] = '\0';
				http_trimField(&sof2, &eof2); eof2[1] = '\0';
				
				add_header(&req->data, sof1, sof2);
				
				if (!strcasecmp((char*)sof1,"Content-Length")) {
					int i;
					if (sscanf((char*)sof2, "%d", &i) == 1) {
						req->data.contentLength = i;
					}
				}
				
				break;
				
			case STATE_START_CONTENT:
				req->state = STATE_GETTING_CONTENT;
			case STATE_GETTING_CONTENT:
				if (!req->data.content) {
					req->data.content = sol;
					req->data.contentReceived = eod - sol + 1;
					req->parsePos = eod - req->buf->data;
				} else {
					req->data.contentReceived += eod - sol;
					req->parsePos = eod - req->buf->data;
				}
				
				if (req->data.contentReceived >= req->data.contentLength) req->state = STATE_COMPLETE;
				
				break;
				
			case STATE_COMPLETE:
				break;
				
			default:
				fprintf(stderr, "%s:%d %s(): unknown state: %d\n", __FILE__, __LINE__, __FUNCTION__, req->state);
				ret = HTE_PARSE;
				goto die;
		}
		if (req->state == STATE_ERROR ||
		    req->state == STATE_GETTING_CONTENT ||
				req->state == STATE_COMPLETE) break;
		
		/* move to next line */
		sol = eol + 1;
		sol++;
		/* only skip 2 if we are using '\r\n' line endings */
		if (sol[-1] == '\r' && *sol == '\n') sol++;
		
		req->parsePos = sol - req->buf->data;
	}
	
	return HTE_NONE;
die:
	req->state = STATE_ERROR;
	return ret;
}

hte http_respond(struct session_info *session) {
	return HTE_UNKNOWN;
}
