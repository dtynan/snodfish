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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>

#include "snodfish.h"

int		timeout;
char	*configfile;

void	usage();

/*
 * All life begins here...
 */
int
main(int argc, char *argv[])
{
	int i, pid, debug, workers = 1;

	opterr = debug = 0;
	timeout = 30;
	configfile = "/etc/snodfish.conf";
	while ((i = getopt(argc, argv, "df:w:")) != EOF) {
		switch (i) {
		case 'f':
			configfile = strdup(optarg);
			break;

		case 'd':
			debug = 1;
			break;

		case 'w':
			if ((workers = atoi(optarg)) < 1 || workers > 100)
				usage();
			break;

		default:
			usage();
			break;
		}
	}
	openlog("snodfish", LOG_PID, LOG_DAEMON);
	listenq_init();
	read_config(0);
	create_listeners();
	exit(0);
	/*
	 * Set up in the background, where appropriate...
	 */
	if (debug) {
		if (workers > 1)
			printf("** Only one worker allowed in debug mode **\n");
		worker();
	} else {
		/*
		 * Close all file descriptors and try to detach from
		 * everything.
		 */
		close(0);
		close(1);
		close(2);
		/*
		 * No signals...
		 */
		signal(SIGTERM, terminate);
		signal(SIGHUP, read_config);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		signal(SIGALRM, SIG_IGN);
		signal(SIGURG, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGCONT, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGXCPU, SIG_IGN);
		signal(SIGXFSZ, SIG_IGN);
		signal(SIGVTALRM, SIG_IGN);
		signal(SIGPROF, SIG_IGN);
		signal(SIGWINCH, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, SIG_IGN);
		for (i = 0; i < workers; i++) {
			/*
			 * Fork off as a daemon.
			 */
			if ((pid = fork()) == -1) {
				perror("snodfish: fork failed");
				exit(1);
			}
			if (pid == 0)
				worker();
		}
	}
	exit(0);
}

/*
 * Force some sort of orderly shutdown...
 */
void
terminate(int sig)
{
	printf("Shutdown called on signal %d\n", sig);
	exit(0);
}

/*
 *
 */
void
usage()
{
	fprintf(stderr, "Usage: snodfish [-d][-f <configfile>][-w <workers>]\n");
	exit(2);
}
