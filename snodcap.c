/*
 * Copyright (c) 2014, Kalopa Research.  All rights reserved.  This is free
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
 *
 * ABSTRACT
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include "snodfish.h"

#define BUFFER_SIZE	4096

struct	speed	{
	int	value;
	int	code;
} speeds[] = {
	{50, B50},
	{75, B75},
	{110, B110},
	{134, B134},
	{150, B150},
	{200, B200},
	{300, B300},
	{600, B600},
	{1200, B1200},
	{1800, B1800},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
	{19200, B19200},
	{38400, B38400},
	{57600, B57600},
	{115200, B115200},
	{230400, B230400},
	{0, 0}
};

int	dev_open(char *, int);
char	*canonical(char *);
void	usage();

/*
 * Let there be light...
 */
int
main(int argc, char *argv[])
{
	int i, n, left_fd, right_fd, speed, verbose;
	char *outfile, *left_dev, *right_dev;
	struct snodfish snod;
	fd_set readfds;
	FILE *outfp;

	speed = B19200;
	outfile = "snodfish.out";
	verbose = 0;
	while ((i = getopt(argc, argv, "s:o:v")) != EOF) {
		switch (i) {
		case 's':
			n = atoi(optarg);
			speed = 0;
			for (i = 0; speeds[i].value != 0; i++) {
				if (n == speeds[i].value) {
					speed = speeds[i].code;
					break;
				}
			}
			if (speed == 0) {
				fprintf(stderr, "?snodgrass: can't grok the baud rate.\n");
				exit(1);
			}
			break;

		case 'o':
			outfile = optarg;
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			usage();
			break;
		}
	}
	/*
	 * Set up the devices and the output file.
	 */
	if ((argc - optind) != 2)
		usage();
	left_dev = canonical(argv[optind++]);
	right_dev = canonical(argv[optind++]);
	if (verbose) {
		printf("Speed: %d\n", speed);
		printf("Left TTY: [%s]\n", left_dev);
		printf("Right TTY: [%s]\n", right_dev);
	}
	if ((outfp = fopen(outfile, "w")) == NULL) {
		fprintf(stderr, "snodcap: ");
		perror(outfile);
		exit(1);
	}
	/*
	 * Open the devices and set the speed and parameters.
	 */
	if ((left_fd = dev_open(left_dev, speed)) < 0) {
		fprintf(stderr, "snodcap: ");
		perror(left_dev);
		exit(1);
	}
	if ((right_fd = dev_open(right_dev, speed)) < 0) {
		fprintf(stderr, "snodcap: ");
		perror(right_dev);
		exit(1);
	}
	if (snod_init(&snod) < 0) {
		perror("snodcap: buffer allocation");
		exit(1);
	}
	/*
	 * Set up our select loop.
	 */
	n = left_fd;
	if (right_fd > n)
		n = right_fd;
	n++;
	FD_ZERO(&readfds);
	while (1) {
		FD_SET(left_fd, &readfds);
		FD_SET(right_fd, &readfds);
		if ((i = select(n, &readfds, NULL, NULL, NULL)) < 0) {
			perror("snodcap: select");
			exit(1);
		}
		/*
		 * What did we catch?
		 */
		snod_stamp(&snod);
		gettimeofday(&snod.tstamp, NULL);
		if (FD_ISSET(left_fd, &readfds)) {
			if ((snod.left_size = read(left_fd, snod.left_buffer, BUFFER_SIZE)) < 0) {
				perror("snodcap: left read failure");
				exit(1);
			}
		}
		if (FD_ISSET(right_fd, &readfds)) {
			if ((snod.right_size = read(right_fd, snod.right_buffer, BUFFER_SIZE)) < 0) {
				perror("snodcap: right read failure");
				exit(1);
			}
		}
		if (verbose)
			printf("%ld.%ld: %ld <-> %ld\n", snod.tstamp.tv_sec, snod.tstamp.tv_usec, snod.left_size, snod.right_size);
		snod_write(outfp, &snod);
	}
	/*
	 * We're done - shut it all down.
	 */
	fclose(outfp);
	close(left_fd);
	close(right_fd);
	exit(0);
}

/*
 * Open the device and set some useful parameters.
 */
int
dev_open(char *devname, int speed)
{
	int fd;
	struct termios tios;

	if ((fd = open(devname, O_RDWR)) < 0)
		return(-1);
	/*
	 * Get and set the tty parameters.
	 */
	if (tcgetattr(fd, &tios) < 0)
		return(-1);
	cfsetspeed(&tios, speed);
	if (tcsetattr(fd, TCSANOW, &tios) < 0)
		return(-1);
	return(fd);
}

/*
 * Convert the device name into a fully-qualified path.
 */
char *
canonical(char *dev)
{
	char *cp;

	if (strncmp(dev, "/dev/", 5) != 0) {
		cp = malloc(strlen(dev) + 6);
		strcpy(cp, "/dev/");
		strcat(cp, dev);
	} else
		cp = strdup(dev);
	return(cp);
}

/*
 * Where did it all go wrong...?
 */
void
usage()
{
	fprintf(stderr, "Usage: snodcap [-s SPEED][-o OFILE][-v] tty1 tty2\n");
	exit(2);
}
