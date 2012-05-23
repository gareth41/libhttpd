#ifndef HTTP_H
#define HTTP_H

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

struct session_info;

enum http_state {
	STATE_START = 0,
	STATE_GETTING_HEADERS,
	STATE_START_CONTENT,
	STATE_GETTING_CONTENT,
	STATE_COMPLETE,
	STATE_ERROR
};

struct http_header {
	unsigned char *name;
	unsigned char *value;
};

struct http_data {
	struct http_header *headers;
	int headerc;
	
	size_t contentLength;
	size_t contentReceived;
	unsigned char *content;
};

struct http_request {
	struct buf *buf;
	size_t parsePos;
	enum http_state state;
	
	unsigned char *method;
	unsigned char *uri;
	unsigned char *httpVersion;
	
	struct http_data data;
};

struct http_response {
	struct buf *buf;
	
	unsigned char *httpVersion;
	int httpCode;
	unsigned char *httpReason;
	
	struct http_data data;
};

hte http_read(struct session_info *session);
hte http_parse(struct session_info *session);
hte http_respond(struct session_info *session);

#endif /* HTTP_H */
