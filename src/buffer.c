/*
 * Copyright (c) 2017, Kalopa Research.  All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this product; see the file COPYING.  If not, write to the
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA RESEARCH "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA RESEARCH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#include "snodfish.h"

struct	buffer	*bfreelist = NULL;

/*
 * Allocate a free buffer. If there's one on the free list, use that.
 * Otherwise, allocate one.
 */
struct buffer *
buf_alloc()
{
	struct buffer *bp;

	if ((bp = bfreelist) != NULL)
		bfreelist = bp->next;
	else if ((bp = (struct buffer *)malloc(sizeof(struct buffer))) == NULL) {
		syslog(LOG_ERR, "buf_alloc() malloc: %m");
		exit(1);
	}
	bp->size = 0;
	bp->next = NULL;
	return(bp);
}

/*
 * Free up a buffer by sticking it on the free list.
 */
void
buf_free(struct buffer *bp)
{
	bp->next = bfreelist;
	bfreelist = bp;
}

/*
 * Flush all the buffers on a given channel.
 */
void
buf_freeall(struct buffer *bp)
{
	struct buffer *nbp;

	while (bp != NULL) {
		nbp = bp->next;
		buf_free(bp);
		bp = nbp;
	}
}

/*
 * Read data for a specific channel into a buffer and queue up the buffer.
 */
int
buf_read(struct channel *chp)
{
	struct buffer *bp = buf_alloc();

	/*
	 * Read from the device.
	 */
	if ((bp->size = read(chp->fd, bp->data, READSIZE)) <= 0) {
		buf_free(bp);
		return(-1);
	}
	/*
	 * Stick the buffer on the end of the channel read queue.
	 */
	chp->totread += bp->size;
	if (chp->rqhead == NULL)
		chp->rqhead = bp;
	else {
		struct buffer *nbp;

		for (nbp = chp->rqhead; nbp->next != NULL; nbp = nbp->next)
			;
		nbp->next = bp;
	}
	return(bp->size);
}

/*
 * Write data from a specific channel to the specified file descriptor.
 * We assume we can write at least READSIZE bytes, or else we'll fail.
 * Otherwise we'd have to deal with partial buffers, and who wants that?
 */
int
buf_write(struct channel *chp)
{
	int n;
	struct buffer *bp;

	/*
	 * Pull the buffer from the head of the queue and write it out
	 * to the specified device.
	 */
	bp = chp->wqhead;
	chp->wqhead = bp->next;
	if ((n = write(chp->fd, bp->data, bp->size)) != bp->size)
		return(-1);
	/*
	 * Release the buffer and we're done.
	 */
	chp->totread -= n;
	buf_free(bp);
	return(n);
}
