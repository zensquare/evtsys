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
#include "eventlog.h"
#include "log.h"
#include "service.h"
#include "syslog.h"
#include "check.h"
#include "dns.h"

/* Number of eventlogs */
#define EVENTLOG_SZ		32

/* Size of buffer */
#define EVENTLOG_BUF_SZ	(10*1024)

/* Number of strings in formatted message */
#define EVENTLOG_ARRAY_SZ	96

/* Eventlog descriptor */
struct Eventlog {
	char name[EVENTLOG_NAME_SZ];	/* Name of eventlog		*/
	HANDLE handle;					/* Handle to eventlog	*/
	char buffer[EVENTLOG_BUF_SZ];	/* Message buffer		*/
	int count;			/* Number of messages left	*/
	int pos;			/* Current position in buffer	*/
	int recnum;			/* Next record number		*/
};

/* List of eventlogs */
static struct Eventlog EventlogList[EVENTLOG_SZ];
int EventlogCount = 0;

/* Create new eventlog descriptor */
int EventlogCreate(char * name)
{
	/* Check count */
	if (EventlogCount == EVENTLOG_SZ) {
		Log(LOG_ERROR, "Too many eventlogs: %d", EVENTLOG_SZ);
		return 1;
	}

	/* Store new name */
	strncpy_s(EventlogList[EventlogCount].name, sizeof(EventlogList[EventlogCount].name), name, _TRUNCATE);

	/* Increment count */
	EventlogCount++;

	/* Success */
	return 0;
}

/* Close eventlog */
static void EventlogClose(int log)
{
	/* Close log */
	CloseEventLog(EventlogList[log].handle);
	EventlogList[log].handle = NULL;
}

/* Close eventlogs */
void EventlogsClose()
{
	int i;

	/* Loop until list depleted */
	for (i = 0; i < EventlogCount; i++)
		if (EventlogList[i].handle)
			EventlogClose(i);

	/* Reset count */
	EventlogCount = 0;
}

/* Open event log */
static int EventlogOpen(int log)
{
	DWORD count;
	DWORD oldest;

	/* Reset all indicators */
	EventlogList[log].count = 0;
	EventlogList[log].pos = 0;
	EventlogList[log].recnum = 1;

	/* Open log */
	EventlogList[log].handle = OpenEventLog(NULL, EventlogList[log].name);
	if (EventlogList[log].handle == NULL) {
		Log(LOG_ERROR|LOG_SYS, "Cannot open event log: \"%s\"", EventlogList[log].name);
		return 1;
	}

	/* Get number of records to skip */
	if (GetNumberOfEventLogRecords(EventlogList[log].handle, &count) == 0) {
		Log(LOG_ERROR|LOG_SYS, "Cannot get record count for event log: \"%s\"", EventlogList[log].name);
		return 1;
	}

	/* Get oldest record number */
	if (GetOldestEventLogRecord(EventlogList[log].handle, &oldest) == 0 && count != 0) {
		Log(LOG_ERROR|LOG_SYS, "Cannot get oldest record number for event log: \"%s\"", EventlogList[log].name);
		return 1;
	}

	/* Store record of next event */
	EventlogList[log].recnum = oldest + count;
	if (EventlogList[log].recnum == 0)
		EventlogList[log].recnum = 1; /* ?? */

	/* Success */
	return 0;
}

/* Open event logs */
int EventlogsOpen()
{
	int i;

	/* Open the log files */
	for (i = 0; i < EventlogCount; i++)
		if (EventlogOpen(i))
			break;

	/* Check for errors */
	if (i != EventlogCount) {
		EventlogsClose();
		return 1;
	}

	/* Success */
	return 0;
}

/* Get the next eventlog message */
char * EventlogNext(EventList ignore_list[MAX_IGNORED_EVENTS], int log, int * level)
{
	BOOL reopen = FALSE;
	DWORD errnum;
	DWORD needed;
	DWORD loglevel;
	EVENTLOGRECORD * event;
	char * cp;
	char * current;
	char * formatted_string;
	char * message_file;
	char * source;
	char * string_array[EVENTLOG_ARRAY_SZ];
	char * username;
	char hostname[HOSTNAME_SZ];
	char defmsg[ERRMSG_SZ];
	int event_id;
	int i;
	char *index;

	static char message[SYSLOG_DEF_SZ-17];
	static char tstamped_message[SYSLOG_DEF_SZ];

	/* Initialize array to prevent memory exceptions with bad message definitions */
	for(i = 0; i < EVENTLOG_ARRAY_SZ; i++)
		string_array[i]="*";

	/* Are there any records left in buffer */
	while (EventlogList[log].pos == EventlogList[log].count) {

		/* Reset input position */
		EventlogList[log].count = 0;
		EventlogList[log].pos = 0;

		/* Read a record */
		needed = 0;
		if (ReadEventLog(EventlogList[log].handle, EVENTLOG_FORWARDS_READ | EVENTLOG_SEEK_READ, EventlogList[log].recnum, EventlogList[log].buffer, sizeof(EventlogList[log].buffer), &EventlogList[log].count, &needed) == 0) {

			/* Check error */
			errnum = GetLastError();
			switch (errnum) {

			/* Message too large... skip over */
			case ERROR_INSUFFICIENT_BUFFER:
				Log(LOG_WARNING, "Eventlog message size too large: \"%s\": %u bytes", EventlogList[log].name, needed);
				EventlogList[log].recnum++;
				break;

			/* Eventlog corrupted (?)... Reopen */
			case ERROR_EVENTLOG_FILE_CORRUPT:
				Log(LOG_INFO, "Eventlog was corrupted: \"%s\"", EventlogList[log].name);
				reopen = TRUE;
				break;

			/* Eventlog files are clearing... Reopen */
			case ERROR_EVENTLOG_FILE_CHANGED:
				Log(LOG_INFO, "Eventlog was cleared: \"%s\"", EventlogList[log].name);
				reopen = TRUE;
				break;

			/* Record not available (yet) */
			case ERROR_INVALID_PARAMETER:
				return NULL;

			/* Normal end of eventlog messages */
			case ERROR_HANDLE_EOF:
				return NULL;

			/* Eventlog probably closing down */
			case RPC_S_UNKNOWN_IF:
				return NULL;

			/* Unknown condition */
			default:
				Log(LOG_ERROR|LOG_SYS, "Eventlog \"%s\" returned error: ", EventlogList[log].name);
				ServiceIsRunning = FALSE;
				return NULL;
			}

			/* Process reopen */
			if (reopen) {
				EventlogClose(log);
				if (EventlogOpen(log)) {
					ServiceIsRunning = FALSE;
					return NULL;
				}
				reopen = FALSE;
			}
		}
	}

	/* Increase record number */
	EventlogList[log].recnum++;

	/* Get position into buffer */
	current = EventlogList[log].buffer + EventlogList[log].pos;

	/* Get pointer to current event record */
	event = (EVENTLOGRECORD *) current;

	/* Advance position */
	EventlogList[log].pos += event->Length;

	/* Get source and event id */
	source = current + sizeof(*event);
	event_id = (int) HRESULT_CODE(event->EventID);
	
	/* Check Event Info Against Ignore List */
	if (IgnoreSyslogEvent(ignore_list, source, event_id)) {
		if (LogInteractive)
			printf("IGNORING_EVENT: SOURCE=%s & ID=%i\n", source, event_id);
		return NULL;
	}
	
	/* Check number of strings */
	if (event->NumStrings > COUNT_OF(string_array)) {

		/* Too many strings */
		Log(LOG_WARNING, "Eventlog message has too many strings to print message: \"%s\": %u strings", EventlogList[log].name, event->NumStrings);
		formatted_string = NULL;
	} else {

		/* Convert strings to arrays */
		cp = current + event->StringOffset;
		for (i = 0; i < event->NumStrings; i++) {
			string_array[i] = cp;
			while (*cp++ != '\0');
		}

		message_file = LookupMessageFile(EventlogList[log].name, source, event->EventID);

		if (message_file == NULL) {
			/* Cannot load resources */
			formatted_string = NULL;
		} else {
			/* Format eventlog message */
			formatted_string = FormatLibraryMessage(message_file, event->EventID, string_array);
		}
	}

	/* Create a default message if resources or formatting didn't work */
	if (formatted_string == NULL) {
		_snprintf_s(defmsg, sizeof(defmsg), _TRUNCATE,
			"(Facility: %u, Status: %s)",
			HRESULT_FACILITY(event->EventID),
			FAILED(event->EventID) ? "Failure" : "Success"
		);
		formatted_string = defmsg;
	}

	/* replace every space in source by underscores */
	index = source;
	while( *index ) {
		if( *index == ' ' ) {
			*index = '_';
		}
		index++;
	}

	/* Format source and event ID number */
	if(SyslogIncludeTag) {
		_snprintf_s(message, sizeof(message), _TRUNCATE,
			"%s: %s: %u: ",
			SyslogTag,
			source,
			HRESULT_CODE(event->EventID)
		);
	} else {
		_snprintf_s(message, sizeof(message), _TRUNCATE,
			"%s: %u: ",
			source,
			HRESULT_CODE(event->EventID)
		);
	}

	/* Convert user */
	if (event->UserSidLength > 0) {
		username = GetUsername((SID *) (current + event->UserSidOffset));
		if (username) {
			strncat_s(message, sizeof(message), username, _TRUNCATE);
			strncat_s(message, sizeof(message), ": ", _TRUNCATE);
		}
	}

	/* Add formatted string to base message */
	strncat_s(message, sizeof(message), formatted_string, _TRUNCATE);

	/* Select syslog level */
	switch (event->EventType) {
		case EVENTLOG_ERROR_TYPE:
			loglevel = SYSLOG_ERR;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;
		case EVENTLOG_WARNING_TYPE:
			loglevel = SYSLOG_WARNING;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;
		case EVENTLOG_INFORMATION_TYPE:
			loglevel = SYSLOG_NOTICE;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;
		case EVENTLOG_AUDIT_SUCCESS:
            strncat_s(message, sizeof(message), "AUDIT_SUCCESS ", _TRUNCATE);
			loglevel = SYSLOG_NOTICE;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;
		case EVENTLOG_AUDIT_FAILURE:
            strncat_s(message, sizeof(message), "AUDIT_FAILURE ", _TRUNCATE);
			loglevel = SYSLOG_ERR;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;

		/* Everything else */
		case EVENTLOG_SUCCESS:
		default:
			loglevel = SYSLOG_NOTICE;
			*level = SYSLOG_BUILD(SyslogFacility, loglevel);
			break;
	}

	/* If event is not being ignored, make sure it is severe enough to be logged */
	if (SyslogLogLevel != 0)
		if (SyslogLogLevel < loglevel-1)
			return NULL;

	/* Add hostname for RFC compliance (RFC 3164) */
	/* if -a then use the fqdn bound to our IP address. If none, use the IP address */
	if (ProgramUseIPAddress == TRUE) {
		strcpy_s(hostname, HOSTNAME_SZ, ProgramHostName);
	} else {
		if (ExpandEnvironmentStrings("%COMPUTERNAME%", hostname, COUNT_OF(hostname)) == 0) {
			strcpy_s(hostname, COUNT_OF(hostname), "HOSTNAME_ERR");
			Log(LOG_ERROR|LOG_SYS, "Cannot expand %COMPUTERNAME%");
	    }
    }
	
	/* Query and Add timestamp from EventLog, add hostname, */
	/* and finally the message to the string */
	_snprintf_s(tstamped_message, sizeof(tstamped_message), _TRUNCATE,
		"%s %s %s",
		TimeToString(event->TimeGenerated),
		hostname,
		message
	);

	/* Return formatted message */
	return tstamped_message;
}

/* Format Timestamp from EventLog */
char * TimeToString(DWORD dw)
{
	time_t tt;
	struct tm stm;
	char result[16];
	static char formatted_result[] = "Mmm dd hh:mm:ss";

	tt = (time_t) dw;
	
	/* Format timestamp string */
	if (localtime_s(&stm, &tt) == 0) {
		strftime(result, sizeof(result), "%b %d %H:%M:%S", &stm);
		if (result[4] == '0') /* Replace leading zero with a space for */
			result[4] = ' ';  /* single digit days so we comply with the RFC */
	} else
		result[0] = '\0';

	strncpy_s(formatted_result, sizeof(result), result, _TRUNCATE);
	
	return formatted_result;
}