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
#include "check.h"
#include "log.h"
#include "syslog.h"
#include <string.h>

/* Number of libaries to load */
#define LIBRARY_SZ		4

/* Length of a file path */
#define PATH_SZ			1024

/* Length of a SID */
#define SID_SZ			256

/* Length of a registry key */
#define KEY_SZ			256

/* Length of a message format */
#define FORMAT_SZ		65535


int EndsWith(const LPWSTR str, const LPWSTR suffix)
{
	size_t lenstr = 0;
	size_t lensuffix = 0;
	if (!str || !suffix)
		return 0;
	lenstr = wcslen(str);

	lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return 0;

	return wcsncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


int StartsWith(const LPWSTR str, const LPWSTR suffix)
{
	size_t lenstr = 0;
	size_t lensuffix = 0;
	if (!str || !suffix)
		return 0;
	lenstr = wcslen(str);

	lensuffix = strlen(suffix);
	if (lensuffix >  lenstr)
		return 0;

	return wcsncmp(str, suffix, lensuffix) == 0;
}

/* Get error message */
void GetError(DWORD err_num, char * message, int len)
{
	/* Attempt to get the message text */
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err_num, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, len, NULL) == 0)
		_snprintf_s(message, len, _TRUNCATE,
			"(Error %u)",
			err_num
		);
}

/* WIDE Get error message */
void GetErrorW(DWORD err_num, WCHAR * message, int len)
{
	/* Attempt to get the message text */
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err_num, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, len, NULL) == 0)
		_snwprintf_s(message, len, _TRUNCATE,
			L"(Error %u)",
			err_num
		);
}

/* Create Timestamp from current local time */
char * GetTimeStamp()
{
	time_t t = time(NULL);
	struct tm stm;
	char result[16];
	static char timestamp[] = "Mmm dd hh:mm:ss";

	/* Format timestamp string */ 
	if (localtime_s(&stm, &t) == 0) {
		strftime(result, sizeof(result), "%b %d %H:%M:%S", &stm); 
		if (result[4] == '0') /* Replace leading zero with a space on */ 
			result[4] = ' ';  /* single digit days so we comply with the RFC */ 
	} else 
		result[0] = '\0'; 
	strncpy_s(timestamp, sizeof(timestamp), result, _TRUNCATE);

	return timestamp;
}

/* Get user name string for a SID */
char * GetUsername(SID * sid)
{
	DWORD domain_len;
	DWORD name_len;
	HRESULT last_err;
	SID_NAME_USE snu;
	char domain[DNLEN+1];
	char id[32];
	char name[UNLEN+1];
	int c;

	static char result[SID_SZ];

	/* Initialize lengths for return buffers */
	name_len = sizeof(name);
	domain_len = sizeof(domain);

	/* Convert SID to name and domain */
	if (LookupAccountSid(NULL, sid, name, &name_len, domain, &domain_len, &snu) == 0) {

		/* Check last error */
		last_err = GetLastError();

		/* Could not convert - make printable version of numbers */
		_snprintf_s(result, sizeof(result), _TRUNCATE,
			"S-%u",
			sid->Revision
		);
		for (c = 0; c < COUNT_OF(sid->IdentifierAuthority.Value); c++)
			if (sid->IdentifierAuthority.Value[c]) {
				_snprintf_s(id, sizeof(id), _TRUNCATE,
					"-%u",
					sid->IdentifierAuthority.Value[c]
				);
				strncat_s(result, sizeof(result), id, _TRUNCATE);
			}
		for (c = 0; c < sid->SubAuthorityCount; c++) {
			_snprintf_s(id, sizeof(id), _TRUNCATE,
				"-%u",
				sid->SubAuthority[c]
			);
			strncat_s(result, sizeof(result), id, _TRUNCATE);
		}

		/* Show error (skipping RPC errors) */
		if (last_err != RPC_S_SERVER_TOO_BUSY)
			Log(LOG_ERROR|LOG_SYS, "Cannot find SID for \"%s\"", result);
	} else
		_snprintf_s(result, sizeof(result), _TRUNCATE,
			"%s\\%s",
			domain,
			name
		);

	/* Result result */
	return result;
}

/* Check Event Against Ignore List */
BOOL IgnoreSyslogEvent(EventList * ignore_list, const char * E_SOURCE, int E_ID)
{
	int i;
	BOOL inList = FALSE;
	BOOL ignoreEvent = FALSE;

	if (LogInteractive)
		Log(LOG_INFO, "Checking source=%s ID=%i", E_SOURCE, E_ID);

	for (i = 0; i < IGNORED_LINES; i++) {
		
		//if(LogInteractive)
		//	Log(LOG_SYS,"Checking source=%s ID=%i", ignore_list[i].source, ignore_list[i].id);
		
		if ((E_ID == ignore_list[i].id || ignore_list[i].wild == TRUE) &&
		   !(_strnicmp(E_SOURCE, ignore_list[i].source, strlen(E_SOURCE))))
			inList = TRUE; /* Event is in the list */
	}

	/* Only ignore if we are not running the ignore file as include only */
	//ignoreEvent = (inList != SyslogIncludeOnly);

	/* Return Result */
	return inList;
}

/* Check Event Against Ignore List */
BOOL WIgnoreSyslogEvent(EventList * ignore_list, const WCHAR * E_SOURCE, int E_ID)
{
	int i;
	BOOL inList = FALSE;

	//if (LogInteractive)
		//printf("Checking source=%S ID=%i\n", E_SOURCE, E_ID);

	for (i = 0; i < IGNORED_LINES; i++) {

		//if(LogInteractive)
			//printf("Checking source=%S ID=%i wcsimp=%i\n", ignore_list[i].wsource, ignore_list[i].id, _wcsicmp(E_SOURCE, ignore_list[i].wsource));

		if ((ignore_list[i].wild || E_ID == ignore_list[i].id) &&
			(ignore_list[i].wildSource  || _wcsicmp(E_SOURCE, ignore_list[i].wsource) == 0))
			inList = TRUE; /* Event is in the list */
	}

	
	/* Return Result */
	return inList;
}

/* Look up message file key */
char * LookupMessageFile(char * logtype, char * source, DWORD eventID)
{
	HKEY hkey;
	DWORD key_size;
	DWORD key_type;
	LONG status;
	char key[KEY_SZ];
	char key_value[KEY_SZ];

	static char result[PATH_SZ];

	/* Get key to registry */
	_snprintf_s(key, sizeof(key), _TRUNCATE,
		"SYSTEM\\CurrentControlSet\\Services\\Eventlog\\%s\\%s",
		logtype,
		source
	);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
		Log(LOG_ERROR, "Cannot find message file key for \"%s\"", key);
		return NULL;
	}

	/* Get message file from registry */
	key_size = sizeof(key_value);
	status = RegQueryValueEx(hkey, "EventMessageFile", NULL, &key_type, key_value, &key_size);
	RegCloseKey(hkey);
	if (status != ERROR_SUCCESS) {
		Log(LOG_ERROR|LOG_SYS, "Cannot find key value \"EventMessageFile\": \"%s\"", key);
		return NULL;
	}

	/* Expand any environmental strings */
	if (key_type == REG_EXPAND_SZ) {
		if (ExpandEnvironmentStrings(key_value, result, sizeof(result)) == 0) {
			Log(LOG_ERROR|LOG_SYS, "Cannot expand string: \"%s\"", key_value);
			return NULL;
		}
	} else
		strncpy_s(result, sizeof(result), key_value, _TRUNCATE);
	key_value[0] = '\0';

	return result;
}

/* Message file storage */
struct MessageFile {
	char path[PATH_SZ];		/* Path to message file		*/
	HINSTANCE library;		/* Handle to library		*/
};

/* List of message files */
static struct MessageFile MessageFile[LIBRARY_SZ];
static int MessageFileCount;

/* Split message file string to individual file paths */
static void SplitMessageFiles(char * message_file)
{
	DWORD len;
	char * ip;
	char * op;

	/* Process each file, seperated by semicolons */
	MessageFileCount = 0;
	ip = message_file;
	do {

		/* Check library count */
		if (MessageFileCount == COUNT_OF(MessageFile)) {
			Log(LOG_ERROR|LOG_SYS, "Message file split into too many paths: %d paths", MessageFileCount);
			break;
		}

		/* Seperate paths */
		op = strchr(ip, ';');
		if (op)
			*op = '\0';

		/* Expand environment strings in path */
		len = ExpandEnvironmentStrings(ip, MessageFile[MessageFileCount].path, sizeof(MessageFile[MessageFileCount].path)-1);
		if (op)
			*op++ = ';';
		if (len == 0) {
			Log(LOG_ERROR|LOG_SYS, "Cannot expand string: \"%s\"", ip);
			break;
		}
		MessageFileCount++;

		/* Go to next library */
		ip = op;
	} while (ip);
}

/* Load message file libraries */
static void LoadMessageFiles()
{
	HRESULT last_err;
	int i;

	/* Process each file */
	for (i = 0; i < MessageFileCount; i++) {

		/* Load library */
		MessageFile[i].library = LoadLibraryEx(MessageFile[i].path, NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (MessageFile[i].library == NULL) {

			/* Check last error */
			last_err = GetLastError();

			/* Something other than a message file missing problem? */
			if (HRESULT_CODE(last_err) != ERROR_FILE_NOT_FOUND)
				Log(LOG_ERROR|LOG_SYS, "Cannot load message file: \"%s\"", MessageFile[i].path);
		}
	}
}

/* Unload message file libraries */
static void UnloadMessageFiles()
{
	int i;

	/* Close libraries */
	for (i = 0; i < MessageFileCount; i++)
		if (MessageFile[i].library)
			FreeLibrary(MessageFile[i].library);
}

/* Format message */
static char * FormatMessageFiles(DWORD event_id, char ** string_array)
{
	DWORD last_err;
	HRESULT status;
	int i;

	static char result[FORMAT_SZ];

	/* Try each library until it works */
	for (i = 0; i < MessageFileCount; i++) {
		/* Format message */
		status = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY, MessageFile[i].library, event_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), result, sizeof(result), (va_list*)string_array);
		result[FORMAT_SZ-1] = '\0';

		if (status)
			return result;
	}

	/* Check last error */
	last_err = GetLastError();

	/* Something other than a message file missing problem? */
	if (last_err != ERROR_MR_MID_NOT_FOUND) {
		Log(LOG_ERROR|LOG_SYS, "Cannot format event ID %u", event_id);
	}

	/* Cannot format */
	return NULL;
}

/* Collapse white space and expand error numbers */
char * CollapseExpandMessage(char * message)
{
	BOOL skip_space = FALSE;
	DWORD errnum;
	char * ip;
	char * np;
	char * op;
	char ch;
	char errstr[ERRMSG_SZ];

	static char result[SYSLOG_DEF_SZ];

	/* Initialize */
	ip = message;
	op = result;
	np = NULL;

	/* Loop until buffer is full */
	while (op - result < sizeof(result)-1) {

		/* Are we at end of input */
		ch = *ip;
		if (ch == '\0') {

			/* Go to next input */
			ip = np;
			np = NULL;

			/* Is this end of input */
			if (ip == NULL)
				break;

			/* Start again */
			continue;
		} else {
			ip++;
		}

		/* Convert whitespace to "space" character */
		if (ch == '\r' || ch == '\t' || ch == '\n')
			ch = ' ';

		/* Is this a "space" character? */
		if (ch == ' ') {

			/* Skip if we're skipping spaces */
			if (skip_space)
				continue;

			/* Start skipping */
			skip_space = TRUE;
		} else {
			/* Stop skipping */
			skip_space = FALSE;
		}

		/* Is this a percent escape? */
		if (ch == '%') {

			/* Is the next character another percent? */
			if (*ip == '%') {

				/* Advance over percent */
				ip++;

				/* Convert input to error number */
				errnum = strtol(ip, &ip, 10);

				/* Format message */
				GetError(errnum, errstr, sizeof(errstr));

				/* Push error message as input */
				np = ip;
				ip = errstr;
				continue;
			}
		}

		/* Add character to output */
		*op++ = ch;
	}

	/* Terminate string */
	*op = '\0';

	/* Return result */
	return result;
}


/* WIDE Collapse white space and expand error numbers */
WCHAR * CollapseExpandMessageW(WCHAR * message)
{
	BOOL skip_space = FALSE;
	DWORD errnum;
	WCHAR * ip;
	WCHAR * np;
	WCHAR * op;
	WCHAR ch;
	WCHAR errstr[ERRMSG_SZ];

	static WCHAR result[SYSLOG_DEF_SZ];

	/* Initialize */
	ip = message;
	op = result;
	np = NULL;

	/* Loop until buffer is full */
	while (op - result < COUNT_OF(result)-1) {

		/* Are we at end of input */
		ch = *ip;
		if (ch == L'\0') {

			/* Go to next input */
			ip = np;
			np = NULL;

			/* Is this end of input */
			if (ip == NULL)
				break;

			/* Start again */
			continue;
		} else {
			ip++;
		}

		/* Convert whitespace to "space" character */
		if (ch == L'\r' || ch == L'\t' || ch == L'\n')
			ch = L' ';

		/* Is this a "space" character? */
		if (ch == L' ') {

			/* Skip if we're skipping spaces */
			if (skip_space)
				continue;

			/* Start skipping */
			skip_space = TRUE;
		} else {
			/* Stop skipping */
			skip_space = FALSE;
		}

		/* Is this a percent escape? */
		if (ch == L'%') {

			/* Is the next character another percent? */
			if (*ip == L'%') {

				/* Advance over percent */
				ip++;

				/* Convert input to error number */
				errnum = wcstol(ip, &ip, 10);

				/* Format message */
				GetErrorW(errnum, errstr, COUNT_OF(errstr));

				/* Push error message as input */
				np = ip;
				ip = errstr;
				continue;
			}
		}

		/* Add character to output */
		*op++ = ch;
	}

	/* Terminate string */
	*op = L'\0';

	/* Return result */
	return result;
}

/* Format library message */
char * FormatLibraryMessage(char * message_file, DWORD event_id, char ** string_array)
{
	static char * message;

	/* Split message file */
	SplitMessageFiles(message_file);

	/* Load files */
	LoadMessageFiles();

	/* Format message */
	message = FormatMessageFiles(event_id, string_array);

	/* Collapse white space */
	if (message)
		message = CollapseExpandMessage(message);

	/* Unload files */
	UnloadMessageFiles();

	/* Return result */
	return message;
}

/* Convert string address to IP */
int ConvertLogHostToIp(char * loghost, char ** ipstr)
{
    in_addr_t ip;
	struct hostent * host;
	struct in_addr ia;

    /* Attempt to convert IP number */
    ip = inet_addr(loghost);
    if (ip == (in_addr_t)(-1)) {

	    /* Attempt to convert host name */
	    host = gethostbyname(loghost);
	    if (host == NULL) {
		    Log(LOG_ERROR, "Invalid log host: \"%s\"", loghost);
		    return 1;
	    }

	    /* Set ip */
	    ip = *(in_addr_t *)host->h_addr;
    }

    /* Convert to IP */
    ia.s_addr = ip;
    *ipstr = inet_ntoa(ia);

    /* Check size */
    if (strlen(*ipstr) > sizeof(SyslogLogHost1)-1) {
	    Log(LOG_ERROR, "Log host address too long: \"%s\"", ipstr);
	    return 1;
    }

    /* Success */
    return 0;
}

char * getConfigPath(){
	DWORD last_error;
	DWORD result;
	DWORD path_size = 1024;
	char* path = malloc(path_size);

	for (;;)
	{
		memset(path, 0, path_size);
		result = GetModuleFileName(0, path, path_size - 1);
		last_error = GetLastError();

		if (0 == result)
		{
			free(path);
			path = 0;
			break;
		}
		else if (result == path_size - 1)
		{
			free(path);
			/* May need to also check for ERROR_SUCCESS here if XP/2K */
			if (ERROR_INSUFFICIENT_BUFFER != last_error)
			{
				path = 0;
				break;
			}
			path_size = path_size * 2;
			path = malloc(path_size);
		}
		else
		{
			break;
		}
	}

	if (!path)
	{
		fprintf(stderr, "Failure: %d\n", last_error);
	}
	else
	{
		int length = 0;

		//Remove the filename
		length = strlen(path);
		path[length - 3] = 'c';
		path[length - 2] = 'f';
		path[length - 1] = 'g';

		printf("path=%s\n", path);
	}
	return path;
}


/* Create a new configuration file */
DWORD CreateConfigFile(char * filename)
{
    FILE *file;

    if (fopen_s(&file, filename, "w") != 0) {
		Log(LOG_ERROR|LOG_SYS,"File could not be created: %s", filename);
		return -1;
	}

	fprintf_s(file, "'!!!!THIS FILE IS REQUIRED FOR THE SERVICE TO FUNCTION!!!!\n'\n");
	fprintf_s(file, "'Comments must start with an apostrophe and\n");
	fprintf_s(file, "'must be the only thing on that line.\n'\n");
	fprintf_s(file, "'Do not combine comments and definitions on the same line!\n'\n");
	fprintf_s(file, "'Format is as follows - EventSource:EventID\n");
	fprintf_s(file, "'Use * as a wildcard to ignore all ID's from a given source\n");
	fprintf_s(file, "'E.g. Security-Auditing:*\n'\n");
	fprintf_s(file, "'In Vista/2k8 and upwards remove the 'Microsoft-Windows-' prefix\n");
    fprintf_s(file, "'In Vista/2k8+ you may also specify custom XPath queries\n");
    fprintf_s(file, "'Format is the word 'XPath' followed by a ':', the event log to search,\n");
    fprintf_s(file, "'followed by a ':', and then the select expression\n");
    fprintf_s(file, "'E.g XPath:Application:<expression>\n'\n");
    fprintf_s(file, "'Details can be found in the readme file at the following location:\n");
    fprintf_s(file, "'https://code.google.com/p/eventlog-to-syslog/downloads/list\n");
	fprintf_s(file, "'**********************:**************************\n");
    fprintf_s(file, "XPath:default:Application:<Select Path=\"Application\">*</Select>\n");
    fprintf_s(file, "XPath:default:Security:<Select Path=\"Security\">*</Select>\n");
    fprintf_s(file, "XPath:default:Setup:<Select Path=\"Setup\">*</Select>\n");
    fprintf_s(file, "XPath:default:System:<Select Path=\"System\">*</Select>\n");

	fclose (file);

    /* SUCCESS */
    return 0;
}