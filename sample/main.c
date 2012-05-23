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
#include <httpd.h>

void client_callback(int rxid, struct xfer_info *info, char *content, int contentLength) {
	char *uri;
	
	printf("New client! %d\n", rxid);
	
	httpd_respond(info, "Testing %d %d %d...\r\n", 1, 2, 3);
	httpd_respond(info, "URI requested: '%s'\r\n", httpd_getURI(info));
	httpd_respond(info, "Host: '%s'\r\n", httpd_getHeader(info, "host"));
}

int main(int argc, char *argv[]) {
	struct httpd_info *httpd;
	hte ret;

	if ((ret = httpd_startServer(8080, client_callback, &httpd)) != HTE_NONE) {
		printf("httpd_startServer() returned %d\n", ret);
		return 1;
	}

	printf("Running!\n");

	sleep(60);

	return 0;
}
