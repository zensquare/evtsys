/*
  Copyright (c) 1998-2007, Purdue University
  All rights reserved.

  Redistribution and use in source and binary forms are permitted provided
  that:

  (1) source distributions retain this entire copyright notice and comment,
      and
  (2) distributions including binaries display the following acknowledgement:

         "This product includes software developed by Purdue University."

      in the documentation or other materials provided with the distribution
      and in all advertising materials mentioning features or use of this
      software.

  The name of the University may not be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

  This software was developed by:
     Curtis Smith

     Purdue University
     Engineering Computer Network
     465 Northwestern Avenue
     West Lafayette, Indiana 47907-2035 U.S.A.

  Send all comments, suggestions, or bug reports to:
     software@ecn.purdue.edu
*/

/* Include files */
#include "main.h"
#include "log.h"
#include "getopt.h"

/* getopt() - Windows doesn't have this */

/* This code is a small re-write from the original version
   found in FreeBSD 4.3, found at /usr/src/lib/libc/stdlib/getopt.c

   The original copyright is:

 Copyright (c) 1987, 1993, 1994
	The Regents of the University of California.  All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
    must display the following acknowledgement:
	This product includes software developed by the University of
	California, Berkeley and its contributors.
 4. Neither the name of the University nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

*/

/* Global variables */
char * GetOptArg;	/* argument associated with option */
int GetOptInd = 1;	/* index into parent argv vector */

/* Local variables */
static int GetOptErr = 1;	/* if error message should be printed */
static int GetOptOpt;		/* character checked for validity */
static int GetOptPtrReset;	/* reset getopt */

#define	GETOPT_BAD_CH	'?'
#define	GETOPT_BAD_ARG	':'
#define	GETOPT_ERR_MSG	""

/* Get next argument option */
int GetOpt(int nargc, char ** nargv, char * ostr)
{
	static char *place = GETOPT_ERR_MSG;	/* option letter processing */
	char *oli;				/* option letter list index */
	int ret;

	if (GetOptPtrReset || !*place) {	/* update scanning pointer */
		GetOptPtrReset = 0;
		if (GetOptInd >= nargc || *(place = nargv[GetOptInd]) != '-') {
			place = GETOPT_ERR_MSG;
			return -1;
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++GetOptInd;
			place = GETOPT_ERR_MSG;
			return -1;
		}
	}					/* option letter okay? */
	if ((GetOptOpt = (int)*place++) == (int)':' || !(oli = strchr(ostr, GetOptOpt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means -1.
		 */
		if (GetOptOpt == (int)'-')
			return -1;
		if (!*place)
			++GetOptInd;
		if (GetOptErr && *ostr != ':')
			Log(LOG_ERROR, "Illegal option -- %c", GetOptOpt);
		return GETOPT_BAD_CH;
	}
	if (*++oli != ':') {			/* don't need argument */
		GetOptArg = NULL;
		if (!*place)
			++GetOptInd;
	}
	else {					/* need an argument */
		if (*place)			/* no white space */
			GetOptArg = place;
		else if (nargc <= ++GetOptInd) {	/* no arg */
			place = GETOPT_ERR_MSG;
			if (*ostr == ':')
				ret = GETOPT_BAD_ARG;
			else
				ret = GETOPT_BAD_CH;
			if (GetOptErr)
				Log(LOG_ERROR, "Option requires an argument -- %c", GetOptOpt);
			return ret;
		}
	 	else				/* white space */
			GetOptArg = nargv[GetOptInd];
		place = GETOPT_ERR_MSG;
		++GetOptInd;
	}
	return GetOptOpt;			/* dump back option letter */
}
