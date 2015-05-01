/*
  This code is a modification of the original Eventlog to Syslog Script written by
  Curtis Smith of Purdue University. The original copyright notice can be found below.
  
  The original program was modified by Sherwin Faria for Rochester Institute of Technology
  in July 2009 to provide bug fixes and add several new features. Additions include
  the ability to ignore specific events, add the event timestamp to outgoing messages,
  a service status file, and compatibility with the new Vista/2k8 Windows Events service.

     Sherwin Faria
	 Rochester Institute of Technology
	 Information & Technology Services Bldg. 10
	 1 Lomb Memorial Drive
	 Rochester, NY 14623 U.S.A.
	 
	Send all comments, suggestions, or bug reports to:
		sherwin.faria@gmail.com
*/
 
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

/* syslog - Windows doesn't have this */

/* This code is an extraction from the original version
   found in FreeBSD 6.2, found at /usr/include/syslog.h

   The original copyright is:

 * Copyright (c) 1982, 1986, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)syslog.h    8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/sys/sys/syslog.h,v 1.26 2005/01/07 02:29:24 imp Exp $
*/

/* Maximum size of a message */
#define SYSLOG_DEF_SZ		1024

/* Masks */
#define SYSLOG_PRI(x)		((x) & 0x07)
#define SYSLOG_FAC(x)		(((x) & 0xf8) >> 3)
#define SYSLOG_BUILD(f,p)	((f) << 3 | (p))

/* Priority codes */
#define	SYSLOG_EMERG		0	/* system is unusable */
#define	SYSLOG_ALERT		1	/* action must be taken immediately */
#define	SYSLOG_CRIT		2	/* critical conditions */
#define	SYSLOG_ERR		3	/* error conditions */
#define	SYSLOG_WARNING		4	/* warning conditions */
#define	SYSLOG_NOTICE		5	/* normal but significant condition */
#define	SYSLOG_INFO		6	/* informational */
#define	SYSLOG_DEBUG		7	/* debug-level messages */

/* Facility codes */
#define	SYSLOG_KERN		0	/* kernel messages */
#define	SYSLOG_USER		1	/* random user-level messages */
#define	SYSLOG_MAIL		2	/* mail system */
#define	SYSLOG_DAEMON	3	/* system daemons */
#define	SYSLOG_AUTH		4	/* authorization messages */
#define	SYSLOG_LPR		6	/* line printer subsystem */
#define	SYSLOG_NEWS		7	/* network news subsystem */
#define	SYSLOG_UUCP		8	/* UUCP subsystem */
#define	SYSLOG_CRON		9	/* clock daemon */
#define SYSLOG_AUTHPRIV	10	/* authorization messages (private) */
#define	SYSLOG_FTP		11	/* ftp daemon */
#define	SYSLOG_NTP		12	/* NTP subsystem */
#define	SYSLOG_SECURITY		13	/* security subsystems (firewalling, etc.) */
#define	SYSLOG_LOCAL0		16	/* reserved for local use */
#define	SYSLOG_LOCAL1		17	/* reserved for local use */
#define	SYSLOG_LOCAL2		18	/* reserved for local use */
#define	SYSLOG_LOCAL3		19	/* reserved for local use */
#define	SYSLOG_LOCAL4		20	/* reserved for local use */
#define	SYSLOG_LOCAL5		21	/* reserved for local use */
#define	SYSLOG_LOCAL6		22	/* reserved for local use */
#define	SYSLOG_LOCAL7		23	/* reserved for local use */

/* Default port and logging priority */
#define SYSLOG_DEF_PORT		514
#define SYSLOG_DEF_FAC		SYSLOG_DAEMON
#define SYSLOG_DEF_FAC_NAME	"daemon"

/* Default status interval */
#define SYSLOG_DEF_INTERVAL 0

/* Default minimum logging level */
#define SYSLOG_DEF_LogLevel 0

/* Hostname size */
#define SYSLOG_HOST_SZ		64
#define MAX_CONFIG_FNAME	128

/* Tag max size */
#define SYSLOG_TAG_SZ		64

/* Application data configuration */
extern char SyslogLogHosts[(SYSLOG_HOST_SZ+1)*6];
extern char SyslogLogHost1[SYSLOG_HOST_SZ+1];
extern char SyslogLogHost2[SYSLOG_HOST_SZ+1];
extern char SyslogLogHost3[SYSLOG_HOST_SZ+1];
extern char SyslogLogHost4[SYSLOG_HOST_SZ+1];
extern char SyslogLogHost5[SYSLOG_HOST_SZ+1];
extern char SyslogLogHost6[SYSLOG_HOST_SZ+1];
extern char SyslogLogHostDhcp[SYSLOG_HOST_SZ+1];
extern char SyslogConfigFile[MAX_CONFIG_FNAME+1];
extern char SyslogTag[SYSLOG_TAG_SZ+1];
extern DWORD SyslogIncludeTag;
extern DWORD SyslogPort;
extern DWORD SyslogPortDhcp;
extern DWORD SyslogFacility;
extern DWORD SyslogStatusInterval;
extern DWORD SyslogQueryDhcp;
extern DWORD SyslogLogLevel;
extern DWORD SyslogIncludeOnly;
extern DWORD SyslogMessageSize;
extern DWORD SyslogEnableTcp;