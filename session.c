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
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "internal.h"
#include "session.h"
#include "interface.h"
#include "http.h"
#include "buf.h"

void *session_handleConnection(void *_session) {
	char err_buf[] = "HTTP/1.1 500 Internal Server Error\r\n";
	struct session_info *session = _session;
	struct httpd_info *httpd;
	hte ret = HTE_NONE;
	
	pthread_detach(pthread_self());
	
	if (!session || !session->httpd) return (void*)-1;
	httpd = session->httpd;
	
	if (!session->xfer.request) {
		if ((session->xfer.request = malloc(sizeof(*session->xfer.request))) == NULL) { ret = HTE_NOMEM; goto die; }
		memset(session->xfer.request, 0, sizeof(*session->xfer.request));
	}
	
	if (!session->xfer.response) {
		if ((session->xfer.response = malloc(sizeof(*session->xfer.response))) == NULL) { ret = HTE_NOMEM; goto die; }
		memset(session->xfer.response, 0, sizeof(*session->xfer.response));
	}
	
	/* read request */
	if (http_read(session) != 0) { ret = HTE_READ; goto die; }
	
	/* prepare asumptions about response */
	session->xfer.response->httpVersion = session->xfer.request->httpVersion;
	session->xfer.response->httpCode = 200;
	session->xfer.response->httpReason = (unsigned char*)"Success";
	
	/* run the callback */
	if (httpd->callback(httpd->rxid++, session, (char*)session->xfer.request->buf->data, session->xfer.request->buf->len) != 0) { ret = HTE_CALLBACK; goto die; }
	
	/* send the response */
	if (http_respond(session) != 0) { ret = HTE_RESPOND; goto die; }
	
	goto done;
die:
	
	/* some sort of 'an-error-occured' callback? check ret! */
	fprintf(stderr, "%s:%d %s(): an error occured (%d)\n", __FILE__, __LINE__, __FUNCTION__, ret);

	send(session->fd, err_buf, sizeof(err_buf), MSG_NOSIGNAL);
	
done:
	
	shutdown(session->fd, SHUT_RDWR);
	close(session->fd);
	
	if (session->xfer.request) {
		if (session->xfer.request->buf) buf_free(session->xfer.request->buf);
		if (session->xfer.request->data.headers) free(session->xfer.request->data.headers);
		free(session->xfer.request);
	}
	if (session->xfer.response) {
		
		if (session->xfer.response->headBuf) buf_free(session->xfer.response->headBuf);
		if (session->xfer.response->buf) buf_free(session->xfer.response->buf);
		
		if (session->xfer.response->data.headers) {
			int i;
			struct http_data *htdata;
			struct http_header *header;
			htdata = &session->xfer.response->data;
			
			for (i = 0; i < htdata->headerc; i++) {
				header = &htdata->headers[i];
				if (header->value == NULL) continue;
				if (header->valueFree == 0) continue;
				free(header->value);
			}
			free(session->xfer.response->data.headers);
		}
		free(session->xfer.response);
	}
	
	free(_session);
	return NULL;
}
