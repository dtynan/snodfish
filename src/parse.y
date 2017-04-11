%{
/*
 * Copyright (c) 2003-2017, Kalopa Research.  All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this product; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA MEDIA LIMITED "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA MEDIA LIMITED BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "snodfish.h"

#define YYDEBUG			1

#define MAXLINELEN		512
#define MAXTOKENLEN		64

int				lineno;
int				nerrors;
char			input[MAXLINELEN+2];
char			*inptr = NULL;
char			*filename;
struct pool		*pool = NULL;
FILE			*fp;

int			yylex();
void		yyerror(const char *);
int			lexopen(char *);
void		lexclose();
int			lexline();
%}

%union	{
	int				nval;
	char			*ival;
	char			*sval;
};

%term	<nval>	VAL
%term	<sval>	STRING
%term	<ival>	IDENT

%term	PCENT PTR ERROR
%term	KW_DELETE KW_GET KW_LISTEN KW_MANAGE KW_NAMESPACE KW_POOL KW_POST
%term	KW_PUT KW_RETURNS KW_SERVICE KW_TIMEOUT

%term	';' ','


%type	<nval>	number
%type	<sval>	string service
%%

config_file:	  /* Empty statement */
				| config_file config_stmnt
				;

config_stmnt:	  listen_stmnt
				| timeout_stmnt
				| manage_stmnt
				| namespace_stmnt
				| pool_stmnt
				| basic_get
				| basic_put
				| basic_post
				| basic_delete
				;

listen_stmnt:	  KW_LISTEN string ';'
				{
					int port = DEFAULT_PORT;
					char *cp, *host = $2;

					printf("LISTEN: [%s]\n", host);
					if ((cp = strchr(host, ':')) != NULL) {
						*cp++ = '\0';
						if ((port = atoi(cp)) < 1 || port > 65535) {
							yyerror("invalid port number");
							break;
						}
					}
					printf("H:[%s], P:%d\n", host, port);
					new_listener(host, port);
				}
				;

timeout_stmnt:	  KW_TIMEOUT number ';'
				{
					printf("Timeout is %d\n", $2);
					timeout = $2;
				}
				;

manage_stmnt:	  KW_MANAGE string ';'
				{
					printf("Manage URL: [%s]\n", $2);
				}
				;

namespace_stmnt:  KW_NAMESPACE string ';'
				{
					struct pool *pp = default_pool();

					printf("NAMESPACE: [%s]\n", $2);
					pp->nspace = parse_url($2);
				}
				;

pool_stmnt:		  KW_POOL pool_name '{' pool_defs '}'
				{
					pool = NULL;
				}
				;

basic_get:		  KW_GET string KW_RETURNS string ';'
				{
					build_route(METHOD_GET, $2, default_pool(), NULL, $4);
				}
				;

basic_put:		  KW_PUT string KW_RETURNS string ';'
				{
					build_route(METHOD_PUT, $2, default_pool(), NULL, $4);
				}
				;

basic_post:		  KW_POST string KW_RETURNS string ';'
				{
					build_route(METHOD_POST, $2, default_pool(), NULL, $4);
				}
				;

basic_delete:	  KW_DELETE string KW_RETURNS string ';'
				{
					build_route(METHOD_DELETE, $2, default_pool(), NULL, $4);
				}
				;

pool_defs:		  pool_def
				| pool_defs pool_def
				;

pool_def:		  KW_SERVICE services ';'
				| KW_NAMESPACE string ';'
				{
					if (pool == NULL) {
						yyerror("no pool defined");
						break;
					}
					pool->nspace = parse_url($2);
				}

				| KW_GET string PTR string ';'
				{
					if (pool == NULL) {
						yyerror("no pool defined");
						break;
					}
					build_route(METHOD_GET, $2, pool, $4, NULL);
				}
				| KW_PUT string PTR string ';'
				{
					if (pool == NULL) {
						yyerror("no pool defined");
						break;
					}
					build_route(METHOD_PUT, $2, pool, $4, NULL);
				}
				| KW_POST string PTR string ';'
				{
					if (pool == NULL) {
						yyerror("no pool defined");
						break;
					}
					build_route(METHOD_POST, $2, pool, $4, NULL);
				}
				| KW_DELETE string PTR string ';'
				{
					if (pool == NULL) {
						yyerror("no pool defined");
						break;
					}
					build_route(METHOD_DELETE, $2, pool, $4, NULL);
				}
				;

services:		  service
				{
					if (pool == NULL)
						yyerror("no pool defined");
					else
						new_service(pool, $1);
				}
				| services ',' service
				{
					if (pool == NULL)
						yyerror("no pool defined");
					else
						new_service(pool, $3);
				}
				;

pool_name:		  STRING
				{
					char *name = $1;

					if (name == NULL || *name == '\0')
						yyerror("invalid pool name (must be at least one character long)");
					else
						pool = new_pool($1);
				}
				;

service:		  STRING
				;

number:			  VAL
				;

string:			  STRING
				;
%%

/*
 * Keyword parsing structure...
 */
struct	keyword	{
	char	*name;
	int	value;
} keywords[] = {
	{"delete",		KW_DELETE},
	{"get",			KW_GET},
	{"listen",		KW_LISTEN},
	{"manage",		KW_MANAGE},
	{"namespace",	KW_NAMESPACE},
	{"pool",		KW_POOL},
	{"post",		KW_POST},
	{"put",			KW_PUT},
	{"returns",		KW_RETURNS},
	{"service",		KW_SERVICE},
	{"timeout",		KW_TIMEOUT},
	{NULL,			0}
};

/*
 *
 */
void
read_config(int sig)
{
	static int only_once = 0;

	if (only_once) {
		/*
		 * Unfortunately, we can't handle reloading config.
		 * YET...
		 */
		return;
	}
	only_once = 1;
	printf("Reading configuration file...\n");
	lexopen(configfile);
	pool_init();
	nerrors = 0;
	yyparse();
	if (nerrors > 0) {
		fprintf(stderr, "?snodfish: exiting due to configuration file errors.\n");
		exit(1);
	}
}

/*
 * Open a source file for subsequent parsing.  Note that the
 * file descriptor, current file name and source line number
 * are globals.  It's too hard to pass anything into YACC and
 * anyway, who cares?
 */
int
lexopen(char *fname)
{
	lexclose();
	if (fname == NULL || strcmp(fname, "-") == 0) {
		fp = stdin;
		filename = strdup("(stdin)");
	} else {
		if ((fp = fopen(fname, "r")) == NULL)
			return(-1);
		filename = strdup(fname);
	}
	printf("Open File: [%s]\n", filename);
	lineno = 0;
	inptr = NULL;
	return(0);
}

/*
 * Close an open source file (if there is one).
 */
void
lexclose()
{
	if (fp != NULL)
		fclose(fp);
	if (filename != NULL)
		free(filename);
	filename = NULL;
}

/*
 * Retrieve a single line from the source file, strip the CR/NL from
 * the end where applicable, and keep an eye on the line numbering.
 */
int
lexline()
{
	char *cp;

	inptr = NULL;
	if (fp == NULL || fgets(input, sizeof(input) - 2, fp) == NULL)
		return(EOF);
	lineno++;
	if ((cp = strpbrk(input, "\r\n")) != NULL)
		*cp = '\0';
#ifdef YYDEBUG
	if (yydebug)
		printf("Line %d: [%s]\n", lineno, input);
#endif
	return(0);
}

/*
 * Return a single lexical token to the parser.  This is done by
 * keeping a pointer into the source line and when the pointer
 * falls off the end, fetch a new line.  Then return the next
 * lexical token in the line.  Some special stuff happens here
 * because some tokens (such as PCENT) begin at the start of a
 * line only.  Anywhere else, the token '%' is returned.
 */
int
yylex()
{
	struct keyword *kp;
	char *cp, token[MAXTOKENLEN+2], endch;

	/*
	 * Skip any leading whitespace.
	 */
	while (inptr != NULL && isspace(*inptr))
		inptr++;
	/*
	 * If we don't have a line in play, then keep reading from the
	 * source file until we have a decent input line or we reach
	 * the end of the file.
	 */
	while (inptr == NULL || *inptr == '\0') {
		if (lexline() == EOF)
			return(EOF);
		/*
		 * Trim leading whitespace on the line.
		 */
		for (cp = input; isspace(*cp); cp++)
			;
		/*
		 * A comment uses a '#' at the start of the line.  If
		 * we see a comment character or a blank line, then
		 * try again.
		 */
		if (*cp == '#' || *cp == '\0')
			continue;
		/*
		 * It's a keeper.  Check for the presence of a
		 * percent at the start of the line.
		 */
		inptr = cp;
		if (*inptr == '%') {
			inptr++;
			return(PCENT);
		}
	}
	/*
	 * We have a valid line with non-whitespace in front.  See
	 * if we can do this the easy way...
	 */
	switch (*inptr) {
	case ';':
	case ',':
	case '.':
	case '(':
	case ')':
	case '{':
	case '}':
	case '/':
		/*
		 * A special character - just return it as-is.
		 */
		return(*inptr++);

	case '=':
		/*
		 * Oops!  We might have a pointer specifier.
		 */
		inptr++;
		if (*inptr == '>') {
			inptr++;
			return(PTR);
		}
		return('-');

	case '"':
	case '\'':
		/*
		 * Seems to be a character string.  Store it up until
		 * we reach the end of it.
		 */
		endch = *inptr++;
		yylval.sval = inptr;
		while (*inptr != '\0' && *inptr != endch) {
			if (*inptr == '\\') {
				inptr++;
				if (*inptr == '\0')
					break;
			}
			inptr++;
		}
		if (*inptr != '\0')
			*inptr++ = '\0';
		return(STRING);

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		/*
		 * A numeric value.  Keep eating digits until we
		 * run out.
		 */
		yylval.nval = 0;
		while (isdigit(*inptr))
			yylval.nval = yylval.nval * 10 + *inptr++ - '0';
		return(VAL);
	}
	/*
	 * OK, at this point it better be some sort of identifier.
	 * An identifier begins with an alpha character or underscore.
	 * If we see anything else in the stream, it's an error.
	 */
	if (!isalpha(*inptr) && *inptr != '_')
		return(ERROR);
	/*
	 * Stuff all the characters we see until some delimiter
	 * into a temporary buffer first.
	 */
	cp = token;
	*cp++ = *inptr++;
	while (cp < &token[MAXTOKENLEN]) {
		if (!isalnum(*inptr) && *inptr != '_')
			break;
		*cp++ = *inptr++;
	}
	*cp = '\0';
	/*
	 * Look through the official list of keywords to see if
	 * we can find this particular entry.
	 */
	for (kp = keywords; kp->name != NULL; kp++)
		if (strcasecmp(token, kp->name) == 0)
			return(kp->value);
	/*
	 * No?  It must be a symbol or identifier or something.
	 * Return it as a string identifier.
	 */
	yylval.ival = strdup(token);
	return(IDENT);
}

/*
 * Generic YACC error function.  Mostly YACC returns errors of the
 * form "syntax error" which isn't useful.  At least we can
 * report the line and filename, which is a small help.
 */
void
yyerror(const char *msg)
{
	if (filename != NULL && lineno > 0)
		printf("\"%s\", line %d: ", filename, lineno);
	printf("%s.\n", msg);
	nerrors++;
}
