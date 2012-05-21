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

#include "internal.h"
#include "interface.h"
#include "server.h"

EXPORT hte httpd_startServer(int listenPort, httpd_callback callback, struct httpd_info **_httpd) {
	struct httpd_info *httpd;
	hte ret;
	
	if (!callback || !_httpd) return HTE_INVALPARAM;
	
	if ((httpd = malloc(sizeof(*httpd))) == NULL) return HTE_NOMEM;
	
	httpd->listenPort = listenPort;
	httpd->callback = callback;
	
	if ((ret = srv_listenStart(httpd)) != HTE_NONE) {
		free(httpd);
		return ret;
	}
	
	*_httpd = httpd;
	
	return HTE_NONE;
}