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
#include <stdarg.h>
#include <string.h>

#include "internal.h"
#include "interface.h"
#include "server.h"
#include "http.h"
#include "session.h"
#include "buf.h"

EXPORT hte httpd_startServer(struct httpd_info **_httpd, int listenPort, httpd_callback callback) {
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

EXPORT char *httpd_getMethod(struct session_info *session) {
	if (!session) return NULL;
	return (char *)session->xfer.request->method;
}
EXPORT char *httpd_getURI(struct session_info *session) {
	if (!session) return NULL;
	return (char *)session->xfer.request->uri;
}
EXPORT char *httpd_getHttpVersion(struct session_info *session) {
	if (!session) return NULL;
	return (char *)session->xfer.request->httpVersion;
}
EXPORT char *httpd_getHeader(struct session_info *session, char *field_name) {
	int i;
	struct http_data *data;
	if (!session || !field_name) return NULL;
	data = &session->xfer.request->data;
	for (i = 0; i < data->headerc; i++) {
		if (data->headers[i].name == NULL) continue;
		if (strcasecmp((char*)data->headers[i].name, field_name)) continue;
		return (char *)data->headers[i].value;
	}
	return NULL;
}

EXPORT hte httpd_addHeader(struct session_info *session, char *field_name, char *field_value_format, ...) {
	struct http_data *data;
	void *p;
	int i;
	char *field_value;
	int field_valueFree;

	if (!session || !field_name) return HTE_INVALPARAM;
	
	field_value = NULL;
	field_valueFree = 0;
	
	if (field_value_format != NULL) {
		for (i = 0; field_value_format[i] != '\0'; i++) {
			if (field_value_format[i] == '%') break;
		}
		
		if (field_value_format[i] == '\0') {
			field_value = field_value_format;
		} else {
			va_list ap;
			int len;
			
			va_start(ap, field_value_format);
			len = vsnprintf(NULL, 0, field_value_format, ap);
			va_end(ap);
			
			if (len > 0) {
				char *p;
				int len2;
				
				if ((p = malloc(sizeof(char) * (len + 1))) == NULL) return HTE_NOMEM;
				
				va_start(ap, field_value_format);
				len2 = vsnprintf(p, len + 1, field_value_format, ap);
				va_end(ap);
				
				if (len != len2) {
					free(p);
					return HTE_UNKNOWN;
				}
				
				field_value = p;
				field_valueFree = 1;
			}
		}
	}

	data = &session->xfer.response->data;
	if ((p = realloc(data->headers, sizeof(*data->headers) * (data->headerc + 1))) == NULL) {
		if (field_value != NULL && field_valueFree) free(field_value);
		return HTE_NOMEM;
	}
	data->headers = p;
	
	memset(&(data->headers[data->headerc]), 0, sizeof(data->headers[data->headerc]));
	data->headers[data->headerc].name = (unsigned char *)field_name;
	data->headers[data->headerc].value = (unsigned char *)field_value;
	data->headers[data->headerc].valueFree = field_valueFree;
	
	data->headerc++;

	return HTE_NONE;
}

EXPORT hte httpd_setHttpCode(struct session_info *session, int code, char *reason) {
	if (!session) return HTE_INVALPARAM;
	
	session->xfer.response->httpCode = code;
	session->xfer.response->httpReason = (unsigned char *)reason;
	
	return HTE_NONE;
}

EXPORT hte httpd_respond(struct session_info *session, char *format, ...) {
	va_list ap;
	hte ret;
	
	if (!session || !format) return HTE_INVALPARAM;
	
	va_start(ap, format);
	ret = httpd_vrespond(session, format, ap);
	va_end(ap);
	
	return ret;
}
EXPORT hte httpd_vrespond(struct session_info *session, char *format, va_list ap) {
	va_list ap2;
	hte ret;
	
	if (!session || !format) return HTE_INVALPARAM;
	
	ret = HTE_NONE;
	
	va_copy(ap2, ap);
	if (vbufcatf(&session->xfer.response->buf, format, ap2) < 0) ret = HTE_RESPOND;
	va_end(ap2);
	
	return ret;
}
EXPORT hte httpd_nrespond(struct session_info *session, char *data, int len) {
	hte ret;
	
	if (!session || !data) return HTE_INVALPARAM;
	if (len == 0) return HTE_NONE;
	
	ret = HTE_NONE;
	
	if (nbufcatf(&session->xfer.response->buf, data, len) != len) ret = HTE_RESPOND;
	
	return ret;
}

EXPORT hte httpd_flush(struct session_info *session) {
	hte ret;
	void *p;
	
	if (!session) return HTE_INVALPARAM;
	
	ret = HTE_NONE;
	if (http_respond(session, 0) != HTE_NONE) ret = HTE_RESPOND;
	
	if ((p = buf_alloc(session->xfer.response->buf, 1)) == NULL) return HTE_NOMEM;
	session->xfer.response->buf = p;
	session->xfer.response->buf->pos = 0;
	session->xfer.response->buf->next = 0;
	session->xfer.response->buf->fd = session->fd;
	
	return ret;
}
