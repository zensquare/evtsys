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

/* Include files */
#include <stdio.h>
#include "main.h"
#include "check.h"
#include "eventlog.h"
#include "log.h"
#include "service.h"
#include "syslog.h"
#include "ver.h"
#include "winevent.h"

/* Main eventlog monitoring loop */
int MainLoop()
{
	char * output = NULL;
	HKEY hkey = NULL;
	int level;
	int log;
	int stat_counter = 0;
    BOOL winEvents;
	FILE *fp = NULL;

    //EventList IgnoredEvents[MAX_IGNORED_EVENTS];
    XPathList * XPathQueries = (XPathList*)calloc(1, sizeof(*XPathQueries));

    /* Check for new Crimson Log Service */
	winEvents = CheckForWindowsEvents();


	/* Grab Ignore List From File */
    if (CheckSyslogIgnoreFile(IgnoredEvents, &XPathQueries, getConfigPath()) < 0)
		return 1;

    /* Determine whether Tag is set */
    if (strlen(SyslogTag) > 0)
        SyslogIncludeTag = TRUE;

	if (winEvents == FALSE)
    {
        /* Gather eventlog names */
        if (RegistryGather())
		    return 1;

	    /* Open all eventlogs */
	    if (EventlogsOpen())
		    return 1;
    }

	/* Service is now running */
	Log(LOG_INFO, "evtsys Eventlog to Syslog Service Started: Version %s (%s-bit)", VERSION,
#ifdef _WIN64
		"64"
#else
		"32"
#endif
	);
	Log(LOG_INFO, "evtsys Flags: LogLevel=%u, IncludeOnly=%s, EnableTcp=%s, IncludeTag=%s, StatusInterval=%u",
		SyslogLogLevel,
        SyslogIncludeOnly ? "True" : "False",
        SyslogEnableTcp ? "True" : "False",
        SyslogIncludeTag ? "True" : "False",
		SyslogStatusInterval
	);

    if (winEvents) {
        if(WinEventSubscribe(XPathQueries, XPATH_COUNT) != ERROR_SUCCESS)
        {
            ServiceIsRunning = FALSE;
        }
    }

	/* Loop while service is running */
	while (ServiceIsRunning)
    {
		/* Process records */
		if (winEvents == FALSE) {
			for (log = 0; log < EventlogCount; log++) {
				/* Loop for all messages */
                while ((output = EventlogNext(IgnoredEvents, log, &level))) {
                    if (output != NULL) {
						if (SyslogSend(output, level)) {
							ServiceIsRunning = FALSE;
							break;
						}
                    }
                }
			}
		}
		
		/* Send status message to inform server that client is active */
		if (SyslogStatusInterval != 0) {
			if (++stat_counter == SyslogStatusInterval*12) { // Because the service loops ~12 times/min
				stat_counter = 0; /* Reset Counter */
				Log(LOG_INFO, "Eventlog to Syslog Service Running");
			}
        }

		/* Sleep five seconds */
		Sleep(5000);
	}

	/* Service is stopped */
	Log(LOG_INFO, "Eventlog to Syslog Service Stopped");

	/* Close eventlogs */
    if (winEvents)
        WinEventCancelSubscribes();

	EventlogsClose();
    SyslogClose();

	/* Success */
	return 0;
}
