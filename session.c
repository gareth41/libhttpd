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
		if ((session->xfer.request = malloc(sizeof(*session->xfer.request))) == NULL) goto die;
		memset(session->xfer.request, 0, sizeof(*session->xfer.request));
	}
	
	if (!session->xfer.response) {
		if ((session->xfer.response = malloc(sizeof(*session->xfer.response))) == NULL) goto die;
		memset(session->xfer.response, 0, sizeof(*session->xfer.response));
	}
	
	if (http_read(session) != 0) { ret = HTE_READ; goto die; }
	
	session->xfer.response->httpVersion = session->xfer.request->httpVersion;
	session->xfer.response->httpCode = 200;
	session->xfer.response->httpReason = (unsigned char*)"Success";
	
	httpd->callback(httpd->rxid++, &session->xfer);
	
	if (http_respond(session) != 0) { ret = HTE_RESPOND; goto die; }
	
	goto done;
die:
	
	/* some sort of 'an-error-occured' callback? check ret! */
	send(session->fd, err_buf, sizeof(err_buf), 0);
	
done:
	
	shutdown(session->fd, SHUT_RDWR);
	close(session->fd);
	
	if (session->xfer.request) {
		if (session->xfer.request->buf) buf_free(session->xfer.request->buf);
		free(session->xfer.request);
	}
	if (session->xfer.response) {
		if (session->xfer.response->buf) buf_free(session->xfer.response->buf);
		free(session->xfer.response);
	}
	
	free(_session);
	return NULL;
}
