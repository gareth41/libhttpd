/*
	libhttpd - a C library to aid serving and responding to HTTP requests

	Copyright (C) 2009 onwards  Attie Grande (attie@attie.co.uk)

	libxbee is free software: you can redistribute it and/or modify it
	under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	libxbee is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with libxbee. If not, see <http://www.gnu.org/licenses/>.
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
#include "server.h"

int srv_listenStart(struct httpd_info *info) {
	hte ret = HTE_NONE;
	int i;
	struct sockaddr_in addrinfo;
	
	if (!info) return HTE_INVALPARAM;
	
	if (info->listenPort < 1 || info->listenPort > 65535) return HTE_INVALPARAM;
	
	/* get some memory */
	if (!info->listen) {
		if ((info->listen = malloc(sizeof(*info->listen))) == NULL) { ret = HTE_NOMEM; goto die; }
		info->listen->fd = -1;
	}
	
	/* create a socket */
	if ((info->listen->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) { ret = HTE_SOCK; goto die; }
	
	i = 1;
	if (setsockopt(info->listen->fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) != 0) { ret = HTE_SOCK; goto die; }
	
	memset(&addrinfo, 0, sizeof(addrinfo));
	addrinfo.sin_family = AF_INET;
	addrinfo.sin_port = htons(info->listenPort);
	addrinfo.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(info->listen->fd, (const struct sockaddr *)&addrinfo, sizeof(addrinfo)) != 0) { ret = HTE_BIND; goto die; }
	
	if (pthread_create(&info->listen->tid, NULL, srv_listenThread, (void*)info) != 0) { ret = HTE_THREAD; goto die; }
	
	return HTE_NONE;
die:
	if (info->listen) {
		struct srv_listenInfo *listen = info->listen;
		info->listen = NULL;
		
		if (listen->fd != -1) {
			
			close(listen->fd);
		}
		
		free(info->listen);
	}
	return ret;
}

void *srv_listenThread(void *_info) {
	struct httpd_info *info = _info;
	
	int EINVALc = 0;
	
	int fd;
	struct sockaddr_in addrinfo;
	socklen_t addrlen;
	
	for (;;) {
		addrlen = sizeof(addrinfo);
		if ((fd = accept(info->listen->fd, (struct sockaddr*)&addrinfo, &addrlen)) < 0) {
			int e = errno;
			
			if (e == EAGAIN || e == EWOULDBLOCK) {
				usleep(5000);
				continue;
			} else if (e == EINVAL && EINVALc++ < 1) {
				usleep(5000);
				continue;
			}
			
			fprintf(stderr, "%s:%d %s(): accept() returned an error...\n\taccept(): %d: '%s'",
			        __FILE__, __LINE__, __FUNCTION__, e, strerror(e));
			break;
		}
		
		/* handle the client */
	}
	
	/* some sort of 'not-listening-anymore' callback? */
	
	return NULL;
}
