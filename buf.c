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

#include "internal.h"
#include "buf.h"

struct buf *buf_alloc(struct buf *_buf, size_t size) {
	size_t tot_size;
	struct buf *buf;
	
	tot_size = size + sizeof(*buf);
	
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
