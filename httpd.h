#ifndef __HTTPD_H
#define __HTTPD_H

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

enum httpd_err {
	HTE_NONE = 0,
	HTE_UNKNOWN = -1,
	HTE_INVALPARAM,
	HTE_NOMEM,
	HTE_SOCK,
	HTE_BIND,
	HTE_ACCEPT,
	HTE_THREAD,
	HTE_READ,
	HTE_PARSE,
	HTE_RESPOND,
};
typedef enum httpd_err hte;

struct httpd_info;
struct xfer_info;

typedef void (*httpd_callback)(int rxid, struct xfer_info *info);

struct httpd_info {
	int listenPort;
	struct srv_listenInfo *listen;
	int rxid;
	httpd_callback callback;
};

struct httpd_info *httpd_startServer(int port, httpd_callback callback);

#endif /* __HTTPD_H */