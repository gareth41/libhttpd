#ifndef SESSION_H
#define SESSION_H

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

#include <sys/socket.h>
#include <arpa/inet.h>

struct xfer_info {
	struct http_request *request;
	struct http_response *response;
};

struct session_info {
	int fd;
	struct sockaddr_in addrinfo;
	socklen_t addrlen;
	pthread_t tid;
	struct httpd_info *httpd;

	struct xfer_info xfer;
};

void *session_handleConnection(void *_session);

#endif /* SESSION_H */
