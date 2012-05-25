#ifndef __HTTPD_H
#define __HTTPD_H

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

#include <stdarg.h>

enum httpd_err {
	HTE_NONE = 0,
	HTE_UNKNOWN = -1,
	HTE_INVALPARAM,
	HTE_NOMEM,
	HTE_SOCK,
	HTE_BIND,
	HTE_LISTEN,
	HTE_ACCEPT,
	HTE_THREAD,
	HTE_READ,
	HTE_WRITE,
	HTE_PARSE,
	HTE_RESPOND,
	HTE_CALLBACK,
};
typedef enum httpd_err hte;

struct httpd_info;
struct xfer_info;

typedef int (*httpd_callback)(int rxid, struct xfer_info *info, char *content, int contentLength);

hte httpd_startServer(struct httpd_info **httpd, int listenPort, httpd_callback callback);

char *httpd_getMethod(struct xfer_info *info);
char *httpd_getURI(struct xfer_info *info);
char *httpd_getHttpVersion(struct xfer_info *info);
char *httpd_getHeader(struct xfer_info *info, char *field_name);

/* if you don't give a 'reason' string, it will be looked up
   if you DO give a 'reason' string, it should NOT need to be free()'d */
hte httpd_setHttpCode(struct xfer_info *info, int code, char *reason);

hte httpd_respond(struct xfer_info *info, char *format, ...);
hte httpd_vrespond(struct xfer_info *info, char *format, va_list ap);
hte httpd_nrespond(struct xfer_info *info, char *data, int len);

#endif /* __HTTPD_H */
