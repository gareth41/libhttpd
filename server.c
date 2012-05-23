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
#include <unistd.h>
#include <pthread.h>

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "internal.h"
#include "interface.h"
#include "server.h"
#include "session.h"

int srv_listenStart(struct httpd_info *httpd) {
	hte ret = HTE_NONE;
	int i;
	struct sockaddr_in addrinfo;
	
	if (!httpd) return HTE_INVALPARAM;
	
	if (httpd->listenPort < 1 || httpd->listenPort > 65535) return HTE_INVALPARAM;
	
	/* get some memory */
	if (!httpd->listen) {
		if ((httpd->listen = malloc(sizeof(*httpd->listen))) == NULL) { ret = HTE_NOMEM; goto die; }
		httpd->listen->fd = -1;
	}
	
	/* create a socket */
	if ((httpd->listen->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) { ret = HTE_SOCK; goto die; }
	
	i = 1;
	if (setsockopt(httpd->listen->fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) != 0) { ret = HTE_SOCK; goto die; }
	
	memset(&addrinfo, 0, sizeof(addrinfo));
	addrinfo.sin_family = AF_INET;
	addrinfo.sin_port = htons(httpd->listenPort);
	addrinfo.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(httpd->listen->fd, (const struct sockaddr *)&addrinfo, sizeof(addrinfo)) != 0) { ret = HTE_BIND; goto die; }
	
	if (listen(httpd->listen->fd, 512) != 0) { ret = HTE_LISTEN; goto die; }
	
	if (pthread_create(&httpd->listen->tid, NULL, srv_listenThread, (void*)httpd) != 0) { ret = HTE_THREAD; goto die; }
	
	return HTE_NONE;
die:
	if (httpd->listen) {
		struct srv_listenInfo *listen = httpd->listen;
		httpd->listen = NULL;
		
		if (listen->fd != -1) {
			
			close(listen->fd);
		}
		
		free(httpd->listen);
	}
	return ret;
}

void *srv_listenThread(void *_httpd) {
	struct httpd_info *httpd = _httpd;
	hte ret = HTE_NONE;
	
	int EINVALc = 0;
	
	struct session_info *session = NULL;
	
	for (;;) {
		if (!session) {
			if ((session = malloc(sizeof(*session))) == NULL) { ret = HTE_NOMEM; break; }
			memset(session, 0, sizeof(*session));
			session->httpd = httpd;
		}
	
		session->addrlen = sizeof(session->addrinfo);
		if ((session->fd = accept(httpd->listen->fd, (struct sockaddr*)&session->addrinfo, &session->addrlen)) < 0) {
			int e = errno;
			
			if (e == EAGAIN || e == EWOULDBLOCK) {
				usleep(5000);
				continue;
			} else if (e == EINVAL && EINVALc++ < 1) {
				usleep(5000);
				continue;
			}
			
			fprintf(stderr, "%s:%d %s(): accept() returned an error...\n\taccept(): %d: '%s'\n",
			        __FILE__, __LINE__, __FUNCTION__, e, strerror(e));
			
			ret = HTE_ACCEPT;
			break;
		}
		
		if (pthread_create(&session->tid, NULL, session_handleConnection, (void*)session) != 0) {
			fprintf(stderr, "%s:%d %s(): pthread_create() returned an error...\n\tpthread_create(): %d: '%s'\n",
			        __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
		} else {
			session = NULL;
		}
	}
	
	/* some sort of 'not-listening-anymore' callback? check ret! */
	
	return NULL;
}
