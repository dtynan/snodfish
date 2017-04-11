/*
 * Copyright (c) 2016, Kalopa Research.  All rights reserved.  This is free
 * software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this product; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA RESEARCH "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL KALOPA RESEARCH BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/select.h>
#include <syslog.h>
#include <string.h>

#include "snodfish.h"

struct channel	*cfreelist = NULL;

/*
 * Allocate a channel. If there's one on the free Q then take that. Otherwise,
 * allocate one from malloc(3).
 */
struct channel *
new_channel()
{
	struct channel *chp;
	static int last_channo = 0;

	if ((chp = cfreelist) != NULL)
		cfreelist = chp->next;
	else if ((chp = (struct channel *)malloc(sizeof(struct channel))) == NULL) {
		syslog(LOG_ERR, "chan_alloc() malloc: %m");
		exit(1);
	}
	chp->next = NULL;
	chp->fd = -1;
	chp->channo = ++last_channo;
	chp->rqhead = chp->wqhead = NULL;
	chp->totread = 0;
	return(chp);
}

/*
 *
 */
void
channel_free(struct channel *chp)
{
	buf_freeall(chp->rqhead);
	buf_freeall(chp->wqhead);
	chp->next = cfreelist;
	cfreelist = chp;
}

/*
 *
 */
void
channel_freeall(struct channel *chp)
{
	struct channel *nchp;

	while (chp != NULL) {
		nchp = chp->next;
		channel_free(chp);
		chp = nchp;
	}
}

/*
 * Read from a channel.
 */
int
channel_read(struct channel *chp)
{
	printf("Reading from ch%d...\n", chp->channo);
	return(0);
}

/*
 * Write to a channel.
 */
int
channel_write(struct channel *chp)
{
	printf("Writing to ch%d...\n", chp->channo);
	return(0);
}
