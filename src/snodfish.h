/*
 * Copyright (c) 2014-2017, Kalopa Research.  All rights reserved.
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
#define MAXPATHARGS		64
#define READSIZE		512
#define DEFAULT_PORT	80

/*
 * The service list manages the active services for a given pool.
 */
struct	service	{
	struct service	*next;
	char			*url;
};

/*
 * The pool list is the full listing of supported pools for this
 * particular service offering.
 */
struct	pool		{
	struct pool		*next;
	char			*name;
	struct path		*nspace;
	struct service	*head;
	struct service	*tail;
};

/*
 * A URL path component can be a string or an identifier
 */
struct	path	{
	struct path		*next;
	int				type;
	int				hash;
	char			*value;
};

#define PATH_STRING		0
#define PATH_IDENT		1

/*
 * The route list translates a path list into a response or method.
 */
struct	route	{
	struct route	*next;
	int				method;
	struct path		*path;
	struct pool		*pool;
	char			*func;
	char			*response;
};

#define METHOD_GET		0
#define METHOD_PUT		1
#define METHOD_POST		2
#define METHOD_DELETE	3

/*
 * All reads and writes are from the buffer queue.
 */
struct	buffer	{
	struct	buffer	*next;
	int				size;
	char			data[READSIZE];
};

/*
 * For every inbound connection, we maintain a channel struct.
 */
struct	channel {
	struct channel	*next;
	struct buffer	*rqhead;
	struct buffer	*wqhead;
	int				channo;
	int				fd;
	int				totread;
};

/*
 * For every listener queue we have a collection of inbound channels.
 */
struct	listenq	{
	struct listenq	*next;
	struct channel	*chans;
	char			*hostname;
	int				port;
	int				fd;
};

/*
 * Prototypes and globals.
 */
extern	int			timeout;
extern	char		*configfile;

void				worker();
void				terminate(int);
void				read_config(int);
struct pool			*default_pool();
void				pool_init();
struct pool			*new_pool(char *);
void				pool_free(struct pool *);
struct service		*new_service(struct pool *, char *);
void				service_free(struct service *);
void				service_freeall(struct service *);
struct route 		*build_route(int, char *, struct pool *, char *, char *);
struct route		*new_route();
void				route_free(struct route *);
struct path			*parse_url(char *);
struct path			*new_path();
void				path_free(struct path *);
void				path_freeall(struct path *);
int					compute_hash(const char *);
void				listenq_init();
struct listenq		*new_listener(char *, int);
void				create_listeners();
void				lq_free(struct listenq *);
void				lq_freeall(struct listenq *);
void				lq_poll();
void				lq_accept(struct listenq *);
void				lq_remove(struct listenq *, struct channel *);
struct channel		*new_channel();
void				channel_free(struct channel *);
void				channel_freeall(struct channel *);
int					channel_read(struct channel *);
int					channel_write(struct channel *);
struct buffer 		*buf_alloc();
void				buf_free();
void				buf_freeall();
void				read_off(int);
void				read_on(int);
void				write_off(int);
void				write_on(int);
