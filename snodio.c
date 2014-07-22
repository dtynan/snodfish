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
#include <sys/time.h>
#include <arpa/inet.h>
/*
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
*/

#include "snodfish.h"

/*
 * Initialize a snodfish record.
 */
int
snod_init(struct snodfish *snodp)
{
	snodp->left_size = snodp->right_size = 0;
	if ((snodp->left_buffer = malloc(SNODBUF_SIZE)) == NULL)
		return(-1);
	if ((snodp->right_buffer = malloc(SNODBUF_SIZE)) == NULL)
		return(-1);
	return(0);
}

/*
 * Set the time stamp.
 */
void
snod_stamp(struct snodfish *snodp)
{
	gettimeofday(&snodp->tstamp, NULL);
}

/*
 * Read a snodfish record from the file.
 */
int
snod_read(FILE *infp, struct snodfish *snodp)
{
	unsigned char header[12];
	uint16_t *sp;
	uint32_t *lp;

	if (fread(header, 12, 1, infp) != 1)
		return(-1);
	lp = (uint32_t *)header;
	sp = (uint16_t *)&header[8];
	snodp->tstamp.tv_sec = ntohl(*lp++);
	snodp->tstamp.tv_usec = ntohl(*lp);
	snodp->left_size = ntohs(*sp++);
	snodp->right_size = ntohs(*sp);
	if (snodp->left_size > SNODBUF_SIZE || snodp->right_size > SNODBUF_SIZE)
		return(-1);
	if (fread(snodp->left_buffer, snodp->left_size, 1, infp) != 1)
		return(-1);
	if (fread(snodp->right_buffer, snodp->right_size, 1, infp) != 1)
		return(-1);
	return(0);
}

/*
 * Write a snodfish record to the log file.
 */
int
snod_write(FILE *outfp, struct snodfish *snodp)
{
	unsigned char header[12];
	uint16_t *sp;
	uint32_t *lp;

	lp = (uint32_t *)header;
	sp = (uint16_t *)&header[8];
	*lp++ = htonl(snodp->tstamp.tv_sec);
	*lp   = htonl(snodp->tstamp.tv_usec);
	*sp++ = htons(snodp->left_size);
	*sp   = htons(snodp->right_size);
	if (fwrite(header, 12, 1, outfp) != 1)
		return(-1);
	if (fwrite(snodp->left_buffer, snodp->left_size, 1, outfp) != 1)
		return(-1);
	if (fwrite(snodp->right_buffer, snodp->right_size, 1, outfp) != 1)
		return(-1);
	return(0);
}
