/*
  Copyright (c) 2009, Rochester Institute of Technology
  All rights reserved.

  Redistribution and use in source and binary forms are permitted provided
  that:

  (1) source distributions retain this entire copyright notice and comment,
      and
  (2) distributions including binaries display the following acknowledgement:

         "This product includes software developed by Rochester Institute of Technology."

      in the documentation or other materials provided with the distribution
      and in all advertising materials mentioning features or use of this
      software.

  The name of the University may not be used to endorse or promote products
  derived from this software without specific prior written permission.

  This software contains code taken from the Eventlog to Syslog service
  developed by Curtis Smith of Purdue University.

  THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

  This software was developed by:
     Sherwin Faria

     Rochester Institute of Technology
     Information and Technology Services
     1 Lomb Memorial Drive, Bldg 10
     Rochester, NY 14623 U.S.A.

  Send all comments, suggestions, or bug reports to:
     sherwin.faria@gmail.com

*/
#define TIMEOUT		5000  // 5 seconds: Defines the timeout interval for the EvtNext call

/* Windows Event Levels */
#define WINEVENT_AUDIT_LEVEL		0
#define WINEVENT_CRITICAL_LEVEL		1
#define WINEVENT_ERROR_LEVEL		2
#define WINEVENT_WARNING_LEVEL		3
#define WINEVENT_INFORMATION_LEVEL	4
#define WINEVENT_VERBOSE_LEVEL		5


/* Size of eventlog name */
#define WIN_EVENTLOG_NAME_SZ	128
#define MAX_SUBSCRIPTIONS		16

/* Prototypes */
DWORD   WinEventSubscribe(XPathList*,int);
void    WinEventCancelSubscribes();
WCHAR*  WinEventTimeToString(ULONGLONG fTime);