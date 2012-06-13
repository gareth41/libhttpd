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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

enum httpd_err {
	HTE_NONE = 0,
	HTE_UNKNOWN = -1,
	HTE_INVALPARAM = -2,
	HTE_NOMEM = -3,
	HTE_SOCK = -4,
	HTE_BIND = -5,
	HTE_LISTEN = -6,
	HTE_ACCEPT = -7,
	HTE_THREAD = -8,
	HTE_READ = -9,
	HTE_WRITE = -10,
	HTE_PARSE = -11,
	HTE_RESPOND = -12,
	HTE_CALLBACK = -13,
};
typedef enum httpd_err hte;

struct httpd_info;
struct session_info;

typedef int (*httpd_callback)(int rxid, struct session_info *session, char *content, int contentLength);

hte httpd_startServer(struct httpd_info **httpd, int listenPort, httpd_callback callback);

char *httpd_getMethod(struct session_info *session);
char *httpd_getURI(struct session_info *session);
char *httpd_getHttpVersion(struct session_info *session);
char *httpd_getHeader(struct session_info *session, char *field_name);

/* if you don't give a 'reason' string, it will be looked up
   if you DO give a 'reason' string, it should NOT need to be free()'d */
hte httpd_setHttpCode(struct session_info *session, int code, char *reason);

hte httpd_addHeader(struct session_info *session, char *field_name, char *field_value_format, ...);

hte httpd_respond(struct session_info *session, char *format, ...);
hte httpd_vrespond(struct session_info *session, char *format, va_list ap);
hte httpd_nrespond(struct session_info *session, char *data, int len);

/* calling this function allows you to respond with a large amount of data
   this function will send any buffered headers to the client, and any existing buffered data
	 after calling this function, data will not be buffered, it will be sent directly to the client */
hte httpd_flush(struct session_info *session);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __HTTPD_H */
