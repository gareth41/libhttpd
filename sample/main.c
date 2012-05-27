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

#include <httpd.h>

/* content! */
int page_index(int rxid, struct session_info *session, char *content, int contentLength) {
	httpd_addHeader(session, "Content-Type", "text/plain");
	
	httpd_respond(session, "Testing %d %d %d...\r\n", 1, 2, 3);
	httpd_respond(session, "URI requested: '%s'\r\n", httpd_getURI(session));
	httpd_respond(session, "Host: '%s'\r\n", httpd_getHeader(session, "host"));
	
	httpd_flush(session);
	
	httpd_respond(session, "This line was not %s\r\n", "buffered");
	
	return 0; /* return non-zero for an internal error (500), otherwise build your own error! e.g. 404 */
}

char content_favicon[] = {
	0x42, 0x4D, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80,
	0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF,
	0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x80, 0x00, 0x7F, 0x70, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x78, 0x70, 0x00, 0xFF, 0xFF, 0xFF, 0xF8,
	0x00, 0xFF, 0xF8, 0x00, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0x70, 0x78, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x70, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x87, 0xFF, 0xF8, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0x80, 0x08, 0x80, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x87, 0x77, 0x8F, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/* ########################################################################## */

/* content list */
struct page {
	char *uri;
	
	/* if a callback is registered, then it is executed */
	httpd_callback callback;
	
	/* if not, then if there is static content registered, then it is sent. mimeType is optional */
	char *content;
	size_t contentLength;
	char *mimeType;
	
} pageList[] = {
	{ "/", page_index },
	{ "/favicon.ico", NULL, content_favicon, sizeof(content_favicon), "image/x-icon"}
};

/* ########################################################################## */

/* content lookup */
int client_callback(int rxid, struct session_info *session, char *content, int contentLength) {
	char *uri;
	int i, l;
	
	if ((uri = httpd_getURI(session)) == NULL) return 1; /* return non-zero for an internal error (500) */
	
	l = sizeof(pageList) / sizeof(*pageList);
	for (i = 0; i < l; i++) {
		if (strcmp(pageList[i].uri, uri)) continue;
		
		/* use a callback if possible */
		if (pageList[i].callback != NULL) {
			return pageList[i].callback(rxid, session, content, contentLength);
			
		/* otherwise use static content */
		} else if (pageList[i].content != NULL && pageList[i].contentLength > 0) {
			if (pageList[i].mimeType != NULL) httpd_addHeader(session, "Content-Type", "%s", pageList[i].mimeType);
			httpd_addHeader(session, "Content-Length", "%d", pageList[i].contentLength);
			httpd_flush(session);
			httpd_nrespond(session, pageList[i].content, pageList[i].contentLength);
			
		/* otherwise it's not implemented! (501 - uri is registerd, but has no content) */
		} else {
			httpd_setHttpCode(session, 501, NULL);
		}
		
		return 0;
	}
	
	httpd_setHttpCode(session, 404, NULL);
	
	return 0; 
}

/* ########################################################################## */

int main(int argc, char *argv[]) {
	struct httpd_info *httpd;
	hte ret;

	if ((ret = httpd_startServer(&httpd, 8080, client_callback)) != HTE_NONE) {
		printf("httpd_startServer() returned %d\n", ret);
		return 1;
	}

	printf("Running for 60 sec!\n");

	sleep(60);

	return 0;
}
