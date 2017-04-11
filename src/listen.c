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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "snodfish.h"

int				maxfds;
fd_set			mrdfdset, rfds;
fd_set			mwrfdset, wfds;

struct listenq	*lfreelist;
struct listenq	*lqhead, *lqtail;

/*
 * Initialize the channel structures, including the array of file descriptors
 * used by select(2).
 */
void
listenq_init()
{
	maxfds = 0;
	lfreelist = lqhead = lqtail = NULL;
	FD_ZERO(&mrdfdset);
	FD_ZERO(&mwrfdset);
}

/*
 * We can listen on one or more ports and addresses.
 */
struct listenq *
new_listener(char *host, int port)
{
	struct listenq *lp;

	if ((lp = lfreelist) != NULL)
		lfreelist = lp->next;
	else if ((lp = (struct listenq *)malloc(sizeof(struct listenq))) == NULL) {
		syslog(LOG_ERR, "new_listener() malloc: %m");
		exit(1);
	}
	lp->next = NULL;
	lp->chans = NULL;
	lp->hostname = (host == NULL) ? NULL : strdup(host);
	lp->port = port;
	lp->fd = -1;
	if (lqtail != NULL)
		lqtail->next = lp;
	else
		lqhead = lp;
	lqtail = lp;
	return(lp);
}

/*
 * Create listeners
 */
void
create_listeners()
{
	struct listenq *lp;

	for (lp = lqhead; lp != NULL; lp = lp->next) {
		printf("Host=%s:%d\n", lp->hostname, lp->port);
	}
}

/*
 *
 */
void
lq_free(struct listenq *lp)
{
	if (lp->fd < 0)
		close(lp->fd);
	channel_freeall(lp->chans);
	if (lp->hostname != NULL)
		free(lp->hostname);
	/* ::FIXME:: remove from lqhead/tail */
	lp->next = lfreelist;
	lfreelist = lp;
}

/*
 *
 */
void
lq_freeall(struct listenq *lp)
{
	struct listenq *nlp;

	while (lp != NULL) {
		nlp = lp->next;
		lq_free(lp);
		lp = nlp;
	}
}

/*
 * Accept a new connection on a listener queue.
 */
void
lq_accept(struct listenq *lp)
{
	socklen_t len;
	struct channel *chp = new_channel();
	struct sockaddr_in sin;

	len = sizeof(struct sockaddr_in);
	if ((chp->fd = accept(lp->fd, (struct sockaddr *)&sin, &len)) < 0) {
		syslog(LOG_ERR, "lq_accept failed: %m");
		return;
	}
	syslog(LOG_INFO, "Received new connection from %s", inet_ntoa(sin.sin_addr));
	read_on(chp->fd);
}

/*
 * Remove a channel from the listen queue.
 */
void
lq_remove(struct listenq *lp, struct channel *chp)
{
	if (lp->chans == chp) {
		/*
		 * Do this the easy way.
		 */
		lp->chans = chp->next;
	} else {
		struct channel *nchp;

		for (nchp = lp->chans; nchp != NULL; nchp = nchp->next) {
			if (nchp->next == chp) {
				nchp->next = chp->next;
				break;
			}
		}
	}
	channel_free(chp);
}

/*
 * Poll (select(2)) the list of channels (and the I/O device) for read/write
 * activity and handle accordingly.
 */
void
lq_poll()
{
	int n, fail;
	struct listenq *lp;
	struct channel *chp, *nchp;
	struct timeval tval, *tvp = NULL;

	/*
	 * Maintain a copy of the file descriptor sets, because it's too hard
	 * to initialize the entire array each time.
	 */
	memcpy((char *)&rfds, (char *)&mrdfdset, sizeof(fd_set));
	memcpy((char *)&wfds, (char *)&mwrfdset, sizeof(fd_set));
	tvp = &tval;
	tvp->tv_sec = timeout;
	tvp->tv_usec = 0;
	if ((n = select(maxfds, &rfds, &wfds, NULL, tvp)) < 0) {
		syslog(LOG_ERR, "select failure in poll: %m");
		exit(1);
	}
	if (n == 0)
		return;
	/*
	 * Check for a new connection on the listen queues. Then check for
	 * read/write data (or events) on the various queues.
	 */
	for (lp = lqhead; lp != NULL; lp = lp->next) {
		if (lp->fd >= 0 && FD_ISSET(lp->fd, &rfds))
			lq_accept(lp);
		for (chp = lp->chans; chp != NULL; chp = nchp) {
			nchp = chp->next;
			if (chp->fd < 0)
				continue;
			fail = 0;
			if (FD_ISSET(chp->fd, &rfds) && channel_read(chp) < 0)
				fail = 1;
			if (!fail && FD_ISSET(chp->fd, &wfds) && channel_write(chp) < 0)
				fail = 1;
			if (fail) {
				/*
				 * Channel has failed. Clean this up and move it to the
				 * free Q.
				 */
				read_off(chp->fd);
				write_off(chp->fd);
				close(chp->fd);
				chp->fd = -1;
				lq_remove(lp, chp);
			}
		}
	}
}

/*
 * Enable read polling on the specified file descriptor.
 */
void
read_on(int fd)
{
	if (fd < 0)
		return;
	if (maxfds <= fd)
		maxfds = fd + 1;
	FD_SET(fd, &mrdfdset);
}

/*
 * Disable read polling on the specified file descriptor.
 */
void
read_off(int fd)
{
	if (fd >= 0)
		FD_CLR(fd, &mrdfdset);
}

/*
 * Enable write polling on the specified file descriptor.
 */
void
write_on(int fd)
{
	if (fd < 0)
		return;
	if (maxfds <= fd)
		maxfds = fd + 1;
	FD_SET(fd, &mwrfdset);
}

/*
 * Disable write polling on the specified file descriptor.
 */
void
write_off(int fd)
{
	if (fd >= 0)
		FD_CLR(fd, &mwrfdset);
}
