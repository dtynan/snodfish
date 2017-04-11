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

struct pool		*pfreelist = NULL;
struct service	*sfreelist = NULL;

struct pool		*pqhead, *pqtail;

/*
 * Initialize the pool list...
 */
void
pool_init()
{
	pqtail = NULL;
	new_pool(NULL);
}

/*
 * Return the default pool name (the one at the top of the queue).
 */
struct pool *
default_pool()
{
	return(pqhead);
}

/*
 * Create a new service pool.
 */
struct pool *
new_pool(char *name)
{
	struct pool *pp;

	if ((pp = pfreelist) != NULL)
		pfreelist = pp->next;
	else if ((pp = (struct pool *)malloc(sizeof(struct pool))) == NULL) {
		syslog(LOG_ERR, "new_pool() malloc: %m");
		exit(1);
	}
	pp->next = NULL;
	if (name == NULL || *name == '\0')
		pp->name = NULL;
	else
		pp->name = strdup(name);
	pp->nspace = NULL;
	pp->head = pp->tail = NULL;
	if (pqtail != NULL)
		pqtail->next = pp;
	else
		pqhead = pp;
	pqtail = pp;
	return(pp);
}

/*
 *
 */
void
pool_free(struct pool *pp)
{
	if (pp->name != NULL)
		free(pp->name);
	path_freeall(pp->nspace);
	service_freeall(pp->head);
	memset(pp, 0, sizeof(struct pool));
	pp->next = pfreelist;
	pfreelist = pp;
}

/*
 * Allocate a new service handler.
 */
struct service *
new_service(struct pool *pp, char *url)
{
	struct service *sp;

	if ((sp = sfreelist) != NULL)
		sfreelist = sp->next;
	else if ((sp = (struct service *)malloc(sizeof(struct service))) == NULL) {
		syslog(LOG_ERR, "new_service() malloc: %m");
		exit(1);
	}
	sp->next = NULL;
	sp->url = strdup(url);
	if (pp->tail != NULL)
		pp->tail->next = sp;
	else
		pp->head = sp;
	pp->tail = sp;
	return(sp);
}

/*
 * Release (free up) a service handler.
 */
void
service_free(struct service *sp)
{
	if (sp->url != NULL)
		free(sp->url);
	sp->url = NULL;
	sp->next = sfreelist;
	sfreelist = sp;
}

/*
 * Free all of the services.
 */
void
service_freeall(struct service *sp)
{
	struct service *nsp;

	while (sp != NULL) {
		nsp = sp->next;
		service_free(sp);
		sp = nsp;
	}
}
