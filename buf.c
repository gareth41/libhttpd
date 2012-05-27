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
	va_list ap;
	int ret;
	
	va_start(ap, format);
	ret = vbufcatf(buf, format, ap);
	va_end(ap);
	
	return ret;
}
int vbufcatf(struct buf **buf, char *format, va_list ap) {
	int l, l1, l2;
	va_list ap2;
	
	if (!buf || !format) return -1;
	
	va_copy(ap2, ap);
	l1 = l = vsnprintf(NULL, 0, format, ap2);
	va_end(ap2);
	if (l <= 0) return l;
	l++; /* <-- space for the nul */
	
	if (*buf == NULL || (*buf)->next + l > (*buf)->len) {
		void *p;
		int n;
		
		if (*buf != NULL) {
			n = (*buf)->next;
		} else {
			n = 0;
		}
		
		if ((p = buf_alloc(*buf, n + l1)) == NULL) return -2;
		*buf = p;
	}
	
	va_copy(ap2, ap);
	l2 = vsnprintf((char*)&((*buf)->data[(*buf)->next]), l, format, ap2);
	va_end(ap2);
	if (l2 <= 0) return l2;
	
	/*            v-- don't forget that nul */
	if (l != l2 + 1) return -3;
	
	(*buf)->next += l2;
	if ((*buf) && (*buf)->fd != 0) {
		l2 = write((*buf)->fd, (*buf)->data, (*buf)->next);
		(*buf)->next = 0;
	}
	
	return l2;
}
int nbufcatf(struct buf **buf, char *data, int len) {
	if (!buf || !data) return -1;
	if (len <= 0) return 0;
	
	if ((*buf) && (*buf)->fd != 0 && (*buf)->next == 0) {
		len = write((*buf)->fd, data, len);
		(*buf)->next = 0;
		return len;
	}
	
	if (*buf == NULL || (*buf)->next + len + 1 > (*buf)->len) {
		void *p;
		int n;
		
		if (*buf != NULL) {
			n = (*buf)->next;
		} else {
			n = 0;
		}
		
		if ((p = buf_alloc(*buf, n + len)) == NULL) return -2;
		*buf = p;
	}
	
	if ((*buf) && (*buf)->fd != 0) {
		len = write((*buf)->fd, data, len);
		(*buf)->next = 0;
	} else {
		memcpy(&((*buf)->data[(*buf)->next]), data, len);
		(*buf)->next += len;
	}
	
	return len;
}

hte buf_send(int fd, struct buf *buf) {
	size_t p,l;
	
	if (!buf || fd == -1) return HTE_WRITE;
	
	for (p = 0; p < buf->len; p += l) {
		if ((l = send(fd, &(buf->data[p]), buf->len - p, MSG_NOSIGNAL)) > 0) continue;
		return HTE_WRITE;
	}
	
	return HTE_NONE;
}
