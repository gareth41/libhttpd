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

#include "internal.h"
#include "session.h"
#include "interface.h"
#include "http.h"

void *session_handleConnection(void *_session) {
	char err_buf[] = "HTTP/1.1 500 Internal Server Error\r\n";
	struct session_info *session = _session;
	struct httpd_info *httpd;
	hte ret = HTE_NONE;
	
	if (!session || !session->httpd) return (void*)-1;
	
	httpd = session->httpd;
	
	if (http_read(session) != 0) { ret = HTE_READ; goto die; }
	
	if (http_parse(session) != 0) { ret = HTE_PARSE; goto die; }
	
	httpd->callback(httpd->rxid++, session->xfer);
	
	if (http_respond(session) != 0) { ret = HTE_RESPOND; goto die; }
	
	goto done;
die:
	
	/* some sort of 'an-error-occured' callback? check ret! */
	send(session->fd, err_buf, sizeof(err_buf), 0);
	
done:
	
	shutdown(session->fd, SHUT_RDWR);
	close(session->fd);
	
	free(_session);
	return NULL;
}
