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
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "internal.h"
#include "buf.h"

struct buf *buf_alloc(struct buf *_buf, size_t size) {
	size_t tot_size;
	struct buf *buf;
	
	tot_size  = sizeof(*buf);
	tot_size += sizeof(char) * size;
	
	if (size <= 0) {
		if (_buf) buf_free(_buf);
		return NULL;
	}
	
	if ((buf = realloc(_buf, tot_size)) == NULL) return NULL;
	
	if (!_buf) {
		/* wipe everything in preparation */
		memset(buf, 0, tot_size);
	} else {
		if (size > buf->len) {
			/* only wipe the new space (if it grew) */
			memset(&(buf->data[buf->len]), 0, size - buf->len + 1);
		}
	}
	buf->len = size;
	
	return buf;
}

void buf_free(struct buf *buf) {
	free(buf);
}


int bufcatf(struct buf **buf, char *format, ...) {
	int l, l2;
	va_list ap;
	
	if (!buf || !format) return -1;
	
	va_start(ap, format);
	l = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	if (l <= 0) return l;
	
	if (*buf == NULL || (*buf)->next + l > (*buf)->len) {
		void *p;
		int n;
		
		if (*buf != NULL) {
			n = (*buf)->next;
		} else {
			n = 0;
		}
		
		if ((p = buf_alloc(*buf, n + l)) == NULL) return -2;
		*buf = p;
	}
	
	va_start(ap, format);
	l2 = vsnprintf((char*)&((*buf)->data[(*buf)->next]), l, format, ap);
	va_end(ap);
	if (l2 <= 0) return l2;
	
	if (l != l2) return -3;
	
	(*buf)->next += l2 - 1;
	return l2;
}

hte buf_send(int fd, struct buf *buf) {
	size_t p,l;
	
	if (!buf || fd == -1) return HTE_WRITE;
	
	for (p = 0; p < buf->len; p += l) {
		if ((l = send(fd, &(buf->data[p]), buf->len - p, 0)) > 0) continue;
		return HTE_WRITE;
	}
	
	return HTE_NONE;
}