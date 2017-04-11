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
#include <ctype.h>

#include "snodfish.h"

struct path		*pathfreelist = NULL;

/*
 * Allocate an individual path element.
 */
struct path *
new_path()
{
	struct path *pathp;

	if ((pathp = pathfreelist) != NULL)
		pathfreelist = pathp->next;
	else if ((pathp = (struct path *)malloc(sizeof(struct path))) == NULL) {
		syslog(LOG_ERR, "new_path() malloc: %m");
		exit(1);
	}
	pathp->next = NULL;
	pathp->hash = 0;
	return(pathp);
}

/*
 * Free an individual path element.
 */
void
path_free(struct path *pathp)
{
	if (pathp->value != NULL)
		free(pathp->value);
	pathp->value = NULL;
	pathp->next = pathfreelist;
	pathfreelist = pathp;
}

/*
 * Free a linked-list of path elements.
 */
void
path_freeall(struct path *pathp)
{
	struct path *npathp;

	while (pathp != NULL) {
		npathp = pathp->next;
		path_free(pathp);
		pathp = npathp;
	}
}

/*
 * Split the URL into components.
 */
struct path *
parse_url(char *url)
{
	int i;
	char *cp, *xp;
	struct path *pathp, *head, *tail = NULL;

	if (*url == '/')
		url++;
	for (i = 0; i < MAXPATHARGS; i++) {
		pathp = new_path();
		if ((cp = strchr(url, '/')) != NULL)
			*cp++ = '\0';
		if (*url == '{') {
			url++;
			pathp->type = PATH_IDENT;
			if ((xp = strchr(url, '}')) != NULL)
				*xp = '\0';
		} else {
			pathp->type = PATH_STRING;
			pathp->hash = compute_hash(url);
		}
		if (*url != '\0')
			pathp->value = strdup(url);
		else
			pathp->value = NULL;
		if (tail != NULL)
			tail->next = pathp;
		else
			head = pathp;
		tail = pathp;
		if ((url = cp) == NULL || *url == '\0')
			break;
	}
	return(head);
}
