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
	
	if ((p = realloc(data->headers, sizeof(*data->headers) * (data->headerc + 1))) == NULL) return HTE_NOMEM;
	data->headers = p;
	
	data->headers[data->headerc].name = field_name;
	data->headers[data->headerc].value = field_value;
	
	data->headerc++;
	
	return HTE_NONE;
}

/* ########################################################################## */

unsigned char asc2bin(unsigned char c) {
	switch (c) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': return c - '0';
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': return c - 'a' + 0x0A;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': return c - 'A' + 0x0A;
	}
	return 0xFF;
}

EXPORT void http_uri_decode(unsigned char *uri) {
	int i, o;
	unsigned char t;
	o = 0;
	for (i = 0; uri[i+o] != '\0'; i++) {
		if (uri[i+o] != '%') {
			if (o > 0) uri[i] = uri[i+o];
			continue;
		}
		if (uri[i+o+1] == '\0' || uri[i+o+2] == '\0') break;
		if ((t = asc2bin(uri[i+o+1])) == 0xFF) continue;
		uri[i]  = (t << 4) & 0xF0;
		if ((t = asc2bin(uri[i+o+2])) == 0xFF) continue;
		uri[i] |= (t     ) & 0x0F;
		o += 2;
	}
	uri[i] = '\0';
}
EXPORT void http_uri_decode2(unsigned char *uri) {
	int i, o;
	unsigned char t, p;
	o = 0;
	for (i = 0; uri[i+o] != '\0'; i++) {
		if (uri[i+o] != '%') {
			if (o > 0) uri[i] = uri[i+o];
			continue;
		}
		if (uri[i+o+1] == '\0' || uri[i+o+2] == '\0') break;
		if ((t = asc2bin(uri[i+o+1])) == 0xFF) continue;
		p  = (t << 4) & 0xF0;
		if ((t = asc2bin(uri[i+o+2])) == 0xFF) continue;
		p |= (t     ) & 0x0F;
		if (p == 0x2F) { /* skip '/' */
			int q;
			for (q = 0; q <= o; q++) uri[i+q] = uri[i+o+q];
			i += 2;
			continue;
		}
		uri[i] = p;
		o += 2;
	}
	uri[i] = '\0';
}

/* ########################################################################## */

/* BE AWARE! during the parse, until http_parse_fixup() is called, all pointers are held as indexes */
hte http_read(struct session_info *session) {
	hte ret = HTE_NONE;
	struct http_request *req;
	ssize_t rxLen;
	void *p;
	
	if (!session || !session->xfer.request) return HTE_INVALPARAM;
	req = session->xfer.request;
	
	req->state = STATE_START;
	
	while (req->state != STATE_COMPLETE) {
		if (req->buf == NULL) {
			if ((req->buf = buf_alloc(NULL, HTTP_BLOCK_SIZE)) == NULL) return HTE_NOMEM;
		} else {
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
	
	if ((p = buf_alloc(req->buf, req->buf->next)) != NULL) {
		req->buf = p;
	} else {
		req->buf->len = req->buf->next;
	}
	
	if ((ret = http_parse_fixup(session)) != HTE_NONE) goto die;
	
	return HTE_NONE;
die:
	return ret;
}

EXPORT hte http_parse(struct session_info *session) {
	hte ret;
	struct http_request *req;
	unsigned char *sol,  *eol;  /* {start|end} of line */
	unsigned char *sof1, *eof1; /* {start|end} of field 1 */
	unsigned char *sof2, *eof2; /* {start|end} of field 2 */
	unsigned char *sod,  *eod;  /* {start|end} of data */
	
	if (!session || !session->xfer.request) return HTE_INVALPARAM;
	req = session->xfer.request;
	
#define INDEXOF(a) (void*)((a) - sod)
	sod = req->buf->data;
	eod = &(req->buf->data[req->buf->next - 1]);
	
	ret = HTE_NONE;
	
	while (req->parsePos < req->buf->next - 1 && req->state != STATE_COMPLETE) {
		sol = &(req->buf->data[req->parsePos]);
		
		if (req->state == STATE_START ||
		    req->state == STATE_PARSING_HEADERS) {
			unsigned char *t;
			/* suck in a line --> sol <==> eol */
			for (eol = sol; *eol != '\r' && *eol != '\n' && eol <= eod; eol++);
			
			/* ran out of data... */
			if (eol >= eod) break;
			if (*eol != '\r' && *eol != '\n') break;

			/* find the start of the next line (remember, '\r' '\r\n' '\n')
			   we have to assume the client is ONLY USING ONE */
			t = eol + 1;
			if (t[-1] == '\r') {
				if (t[0] == '\n') t++;
			} else if (t[-1] == '\n') {
				if (t[0] == '\r') t++;
			}
			req->parsePos = t - req->buf->data;
			*eol = '\0';
		}
		
		switch (req->state) {
			case STATE_START:
				sof1 = sol;
				
				/* get the method */
				http_getField(sof1, &eof1, eod, ' ');
				if (eof1 == NULL) { ret = HTE_PARSE; goto die; };
				req->method = INDEXOF(sof1);
				sof1 = eof1 + 2;
				
				/* get the uri */
				http_getField(sof1, &eof1, eod, ' ');
				if (eof1 == NULL) { ret = HTE_PARSE; goto die; };
				http_uri_decode2(sof1);
				req->uri = INDEXOF(sof1);
				sof1 = eof1 + 2;
				
				/* get the http version */
				if (eof1 + 2 >= eol) { ret = HTE_PARSE; goto die; }
				req->httpVersion = INDEXOF(eof1 + 2);
				
				req->state = STATE_PARSING_HEADERS;
				
				break;
				
			case STATE_PARSING_HEADERS:
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
				
				add_header(&req->data, INDEXOF(sof1), INDEXOF(sof2));
				
				if (!strcasecmp((char*)sof1,"Content-Length")) {
					int i;
					if (sscanf((char*)sof2, "%d", &i) == 1) {
						req->data.contentLength = i;
					}
				}
				
				break;
				
			case STATE_START_CONTENT:
				req->state = STATE_PARSING_CONTENT;
			case STATE_PARSING_CONTENT:
				if (!req->data.content) {
					req->data.content = INDEXOF(sol);
					req->data.contentReceived = eod - sol + 1;
					req->parsePos = eod - req->buf->data;
				} else {
					req->data.contentReceived += eod - sol;
					req->parsePos = eod - req->buf->data;
				}
				
				if (req->data.contentReceived > req->data.contentLength) req->data.contentReceived = req->data.contentLength;
				if (req->data.contentReceived == req->data.contentLength) req->state = STATE_COMPLETE;
				
				break;
				
			case STATE_COMPLETE:
				break;
				
			case STATE_ERROR:
				fprintf(stderr, "%s:%d %s(): for some reason the parser is in an error state...\n", __FILE__, __LINE__, __FUNCTION__);
				ret = HTE_PARSE;
				goto die;
				
			default:
				fprintf(stderr, "%s:%d %s(): unknown state: %d\n", __FILE__, __LINE__, __FUNCTION__, req->state);
				ret = HTE_PARSE;
				goto die;
		}
		if (req->state == STATE_ERROR ||
		    req->state == STATE_PARSING_CONTENT ||
		    req->state == STATE_COMPLETE) break;
	}
#undef INDEXOF
	
	return HTE_NONE;
die:
	req->state = STATE_ERROR;
	return ret;
}
EXPORT hte http_parse_fixup(struct session_info *session) {
	struct http_request *req;
	int i;
	
	if (!session || !session->xfer.request) return HTE_INVALPARAM;
	req = session->xfer.request;
	
#define FIXUP(a) a += (long)req->buf->data
	FIXUP(req->method);
	FIXUP(req->uri);
	FIXUP(req->httpVersion);
	for (i = 0; i < req->data.headerc; i++) {
		if (!req->data.headers[i].name) continue;
		FIXUP(req->data.headers[i].name);
		if (!req->data.headers[i].value) continue;
		FIXUP(req->data.headers[i].value);
	}
	if (req->data.content != NULL) FIXUP(req->data.content);
#undef FIXUP

	return HTE_NONE;
}

hte http_respond(struct session_info *session, int generate_content_length) {
	hte ret;
	int l, i;
	int gotContentLength = 0;
	char *reason;
	
	struct http_response *rsp;
	
	ret = HTE_NONE;
	
	if (!session || !session->xfer.response) return HTE_INVALPARAM;
	rsp = session->xfer.response;
	if (rsp->buf != NULL && rsp->buf->fd != 0) return HTE_NONE;
	
	reason = (char*)rsp->httpReason;
	if (!reason) {
		/* if we don't have a reason string, look it up */
		switch (rsp->httpCode) {
			case 100: reason = "Continue";                        break;
			case 101: reason = "Switching Protocols";             break;
			case 200: reason = "OK";                              break;
			case 201: reason = "Created";                         break;
			case 202: reason = "Accepted";                        break;
			case 203: reason = "Non-Authoritative Information";   break;
			case 204: reason = "No Content";                      break;
			case 205: reason = "Reset Content";                   break;
			case 206: reason = "Partial Content";                 break;
			case 300: reason = "Multiple Choices";                break;
			case 301: reason = "Moved Permanently";               break;
			case 302: reason = "Found";                           break;
			case 303: reason = "See Other";                       break;
			case 304: reason = "Not Modified";                    break;
			case 305: reason = "Use Proxy";                       break;
			case 307: reason = "Temporary Redirect";              break;
			case 400: reason = "Bad Request";                     break;
			case 401: reason = "Unauthorized";                    break;
			case 402: reason = "Payment Required";                break;
			case 403: reason = "Forbidden";                       break;
			case 404: reason = "Not Found";                       break;
			case 405: reason = "Method Not Allowed";              break;
			case 406: reason = "Not Acceptable";                  break;
			case 407: reason = "Proxy Authentication Required";   break;
			case 408: reason = "Request Time-out";                break;
			case 409: reason = "Conflict";                        break;
			case 410: reason = "Gone";                            break;
			case 411: reason = "Length Required";                 break;
			case 412: reason = "Precondition Failed";             break;
			case 413: reason = "Request Entity Too Large";        break;
			case 414: reason = "Request-URI Too Large";           break;
			case 415: reason = "Unsupported Media Type";          break;
			case 416: reason = "Requested range not satisfiable"; break;
			case 417: reason = "Expectation Failed";              break;
			case 500: reason = "Internal Server Error";           break;
			case 501: reason = "Not Implemented";                 break;
			case 502: reason = "Bad Gateway";                     break;
			case 503: reason = "Service Unavailable";             break;
			case 504: reason = "Gateway Time-out";                break;
			case 505: reason = "HTTP Version not supported";      break;
			default:  reason = "Unknown";                         break;
		}
	}
	
	/* add the HTTP status line */
	if ((l = bufcatf(&rsp->headBuf, "%s %d %s\r\n", rsp->httpVersion, rsp->httpCode, reason)) <= 0) { ret = HTE_RESPOND; goto die; }
	
	/* add the headers */
	for (i = 0; i < rsp->data.headerc; i++) {
		if (rsp->data.headers[i].name == NULL) continue;
		if (!strcasecmp((char*)rsp->data.headers[i].name, "Content-Length")) gotContentLength = 1;
		if (rsp->data.headers[i].value == NULL) {
			if ((l = bufcatf(&rsp->headBuf, "%s\r\n", rsp->data.headers[i].name)) <= 0) { ret = HTE_RESPOND; goto die; }
		} else {
			if ((l = bufcatf(&rsp->headBuf, "%s: %s\r\n", rsp->data.headers[i].name, rsp->data.headers[i].value)) <= 0) { ret = HTE_RESPOND; goto die; }
		}
	}
	if (gotContentLength == 0 && generate_content_length != 0) {
		if (rsp->buf) {
			if ((l = bufcatf(&rsp->headBuf, "Content-Length: %d\r\n", rsp->buf->len)) <= 0) { ret = HTE_RESPOND; goto die; }
		} else {
			if ((l = bufcatf(&rsp->headBuf, "Content-Length: 0\r\n")) <= 0) { ret = HTE_RESPOND; goto die; }
		}
	}
	
	/* add the blank line */
	if ((l = bufcatf(&rsp->headBuf, "\r\n")) <= 0) { ret = HTE_RESPOND; goto die; }
	
	if (rsp->headBuf) if ((ret = buf_send(session->fd, rsp->headBuf)) != HTE_NONE) goto die;
	if (rsp->buf)     if ((ret = buf_send(session->fd, rsp->buf))     != HTE_NONE) goto die;
	
	return HTE_NONE;
die:
	return ret;
}
