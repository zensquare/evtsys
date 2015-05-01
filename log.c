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
#include "main.h"
#include "log.h"
#include "syslog.h"
#include "wsock.h"
#include <io.h>

/* Indicate if interactive logging is available */
int LogInteractive = 0;

/* Indicate if eventlog is initialized */
static HANDLE LogSource = NULL;

/* Start using eventlog */
int LogStart()
{
	/* Indicate if interactive logging is available */
	LogInteractive = _isatty(_fileno(stdout));

	/* Open connection to event logger */
	LogSource = RegisterEventSource(NULL, "EvtSys");
	if (LogSource == NULL) {
		Log(LOG_ERROR|LOG_SYS, "Cannot register source for event logging");
		return 1;
	}

	/* Success */
	return 0;
}

/* Stop using eventlog */
void LogStop()
{
	/* Check indicator */
	if (LogSource != NULL) {

		/* Deregister source */
		DeregisterEventSource(LogSource);

		/* Reset indicators */
		LogSource = NULL;
	}
}

/* Send a message to the eventlog */
static int LogSend(WORD level, char * message)
{
	char * messages[1];

	/* Check that the event log is open */
	if (LogSource) {

		/* Set up array */
		messages[0] = message;

		/* Process event */
		if (ReportEvent(LogSource, level, 0, 1, NULL, COUNT_OF(messages), 0, messages, NULL) == FALSE)
			return 1;
	}

	/* Success */
	return 0;
}

/* Print out an error message */
void Log(int level, char * message, ...)
{
	WORD eventlog_priority;
	char hostname[HOSTNAME_SZ];
	char windows_message[ERRMSG_SZ];
	char error_message[SYSLOG_DEF_SZ-17];
	char tstamped_message[SYSLOG_DEF_SZ];
	int syslog_level;
	va_list args;

	static BOOL logging = FALSE;

	/* This prevents recursive errors */
	if (logging)
		return;
	logging = TRUE;

	/* Format and output system message */
	if (level & LOG_SYS)
		GetError(GetLastError(), windows_message, sizeof(windows_message));

	/* Format and output message */
	va_start(args, message);
	vsnprintf_s(error_message, sizeof(error_message), _TRUNCATE, message, args);
	va_end(args);

	/* Append system error message */
	if (level & LOG_SYS) {

		/* Remove bit */
		level &= ~LOG_SYS;

		/* Add windows error message */
		strncat_s(error_message, sizeof(error_message), ": ", _TRUNCATE);
		strncat_s(error_message, sizeof(error_message), windows_message, _TRUNCATE);
	}

	/* Convert local level to eventlog or syslog priority */
	switch (level) {
	case LOG_ERROR:
		eventlog_priority = EVENTLOG_ERROR_TYPE;
		syslog_level = SYSLOG_BUILD(SyslogFacility, SYSLOG_ERR);
		break;
	case LOG_WARNING:
		eventlog_priority = EVENTLOG_WARNING_TYPE;
		syslog_level = SYSLOG_BUILD(SyslogFacility, SYSLOG_WARNING);
		break;
	case LOG_INFO:
		eventlog_priority = EVENTLOG_INFORMATION_TYPE;
		syslog_level = SYSLOG_BUILD(SyslogFacility, SYSLOG_NOTICE);
		break;
	}

	/* Add hostname for RFC compliance (RFC 3164) */
	if (ProgramUseIPAddress == TRUE) {
		strcpy_s(hostname, HOSTNAME_SZ, ProgramHostName);
	} else {
		if (ExpandEnvironmentStrings("%COMPUTERNAME%", hostname, COUNT_OF(hostname)) == 0) {
			strcpy_s(hostname, COUNT_OF(hostname), "HOSTNAME_ERR");
			Log(LOG_ERROR|LOG_SYS, "Cannot expand %COMPUTERNAME%");
        }
	}

	/* Create Timestamp and add to error_message along with hostname */
	/* This maintains consistency with regular non-error packets */
    if(SyslogIncludeTag)
    {
	    _snprintf_s(tstamped_message, sizeof(tstamped_message), _TRUNCATE,
            "%s %s %s: %s",
		    GetTimeStamp(),
		    hostname,
            SyslogTag,
		    error_message
	    );
    }
    else
    {
        _snprintf_s(tstamped_message, sizeof(tstamped_message), _TRUNCATE,
		    "%s %s %s",
		    GetTimeStamp(),
		    hostname,
		    error_message
	    );
    }

	/* Send to syslog if network running */
	if (SyslogSend(tstamped_message, syslog_level))
	{
		/* Otherwise, send to eventlog */
		LogSend(eventlog_priority, tstamped_message);
	}

	/* Output to console */
	if (LogInteractive) {
		fputs(tstamped_message, stderr);
		fputc('\n', stderr);
	}

	/* Okay to log again */
	logging = FALSE;
}
