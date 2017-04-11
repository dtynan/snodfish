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

struct route		*rfreelist = NULL;

/*
 * Allocate a new route.
 */
struct route *
new_route()
{
	struct route *rp;

	if ((rp = rfreelist) != NULL)
		rfreelist = rp->next;
	else if ((rp = (struct route *)malloc(sizeof(struct route))) == NULL) {
		syslog(LOG_ERR, "new_route() malloc: %m");
		exit(1);
	}
	rp->next = NULL;
	return(rp);
}

/*
 * Release an unused route.
 */
void
route_free(struct route *rp)
{
	path_freeall(rp->path);
	if (rp->func != NULL)
		free(rp->func);
	if (rp->response != NULL)
		free(rp->response);
	rp->next = rfreelist;
	rfreelist = rp;
}

/*
 *
 */
struct route *
build_route(int method, char *url, struct pool *pp, char *func, char *response)
{
	struct route *rp = new_route();

	rp->method = method;
	rp->path = parse_url(url);
	rp->pool = pp;
	if (func != NULL)
		rp->func = strdup(func);
	else if (response != NULL)
		rp->response = strdup(response);
	return(rp);
}
