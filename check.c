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

// Include files //
#include "main.h"
#include "log.h"
#include "syslog.h"
#include "winevent.h"
#include "check.h"

int IGNORED_LINES = 0;
int XPATH_COUNT = 0;

// Facility conversion table //
static struct {
	char * name;
	int id;
} FacilityTable[] = {
	{ "auth", SYSLOG_AUTH },
	{ "authpriv", SYSLOG_AUTHPRIV },
	{ "cron", SYSLOG_CRON },
	{ "daemon", SYSLOG_DAEMON },
	{ "ftp", SYSLOG_FTP },
	{ "kern", SYSLOG_KERN },
	{ "local0", SYSLOG_LOCAL0 },
	{ "local1", SYSLOG_LOCAL1 },
	{ "local2", SYSLOG_LOCAL2 },
	{ "local3", SYSLOG_LOCAL3 },
	{ "local4", SYSLOG_LOCAL4 },
	{ "local5", SYSLOG_LOCAL5 },
	{ "local6", SYSLOG_LOCAL6 },
	{ "local7", SYSLOG_LOCAL7 },
	{ "lpr", SYSLOG_LPR },
	{ "mail", SYSLOG_MAIL },
	{ "news", SYSLOG_NEWS },
	{ "ntp", SYSLOG_NTP },
	{ "security", SYSLOG_SECURITY },
	{ "user", SYSLOG_USER },
	{ "uucp", SYSLOG_UUCP }
};

// Check the minimum log level //
int CheckSyslogLogLevel(char * level)
{
	DWORD LogLevel;

	// Convert interval to integer //
	LogLevel = atoi(level);

	// Check for valid number //
	if (LogLevel < MIN_LOG_LEVEL || LogLevel > MAX_LOG_LEVEL) {
		Log(LOG_ERROR, "Bad level: %s \nMust be between %i and %i", level, MIN_LOG_LEVEL, MAX_LOG_LEVEL);
		return 1;
	}

	// Store new value //
	SyslogLogLevel = LogLevel;

	// Success //
	return 0;
}

// Check facility name //
int CheckSyslogFacility(char * facility)
{
	int i;

	// Try looking up name //
	for (i = 0; i < COUNT_OF(FacilityTable); i++)
		if (_stricmp(FacilityTable[i].name, facility) == 0)
			break;
	if (i == COUNT_OF(FacilityTable)) {
		Log(LOG_ERROR, "Invalid facility name: \"%s\"", facility);
		return 1;
	}

	// Store new value //
	SyslogFacility = FacilityTable[i].id;

	// Success //
	return 0;
}

// Check port number //
int CheckSyslogPort(char * port)
{
	DWORD value;
	char * eos;
	struct servent * service;

	// Try converting to integer //
	value = strtoul(port, &eos, 10);
	if (eos == port || *eos != '\0') {

		// Try looking up name //
		service = getservbyname(port, "udp");
		if (service == NULL) {
			Log(LOG_ERROR, "Invalid service name: \"%s\"", port);
			return 1;
		}

		// Convert back to host order //
		value = ntohs(service->s_port);
	} else {

		// Check for valid number //
		if (value <= 0 || value > 0xffff) {
			Log(LOG_ERROR, "Invalid service number: %u", value);
			return 1;
		}
	}

	// Store new value //
	SyslogPort = value;

	// Success //
	return 0;
}

// Check log host //
int CheckSyslogLogHost(char * loghostarg)
{
	char * ipstr = NULL;
    char * loghost = NULL;
    char * next_token = NULL;
    char delim[] = ";";

    // Store new value //
    strncpy_s(SyslogLogHosts, sizeof(SyslogLogHosts), loghostarg, _TRUNCATE);

    // Need to clean up the whole host storage mechanism
    // so much duplication is unacceptable
    loghost = strtok_s(loghostarg, delim, &next_token);
    if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	    strncpy_s(SyslogLogHost1, sizeof(SyslogLogHost1), ipstr, _TRUNCATE);
    else
        return 1;

    loghost = strtok_s(NULL, delim, &next_token);
    if (loghost)
    {
        if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	        strncpy_s(SyslogLogHost2, sizeof(SyslogLogHost2), ipstr, _TRUNCATE);
        else
            return 1;
    }

    loghost = strtok_s(NULL, delim, &next_token);
    if (loghost)
    {
        if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	        strncpy_s(SyslogLogHost3, sizeof(SyslogLogHost3), ipstr, _TRUNCATE);
        else
            return 1;
    }

    loghost = strtok_s(NULL, delim, &next_token);
    if (loghost)
    {
        if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	        strncpy_s(SyslogLogHost4, sizeof(SyslogLogHost4), ipstr, _TRUNCATE);
        else
            return 1;
    }

    loghost = strtok_s(NULL, delim, &next_token);
    if (loghost)
    {
        if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	        strncpy_s(SyslogLogHost5, sizeof(SyslogLogHost5), ipstr, _TRUNCATE);
        else
            return 1;
    }

    loghost = strtok_s(NULL, delim, &next_token);
    if (loghost)
    {
        if (ConvertLogHostToIp(loghost, &ipstr) == 0)
	        strncpy_s(SyslogLogHost6, sizeof(SyslogLogHost6), ipstr, _TRUNCATE);
        else
            return 1;
    }

	// Success //
	return 0;
}

// Check ignore file //
int CheckSyslogIgnoreFile(EventList * ignore_list, XPathList ** xpath_queries, char * filename)
{
	FILE *file;
    int err;
	
    err = fopen_s(&file, filename, "r");

    if (err == ENOENT) {
        Log(LOG_INFO,"Creating file with filename: %s", filename);

		if (CreateConfigFile(filename) != 0)
            return -1;

        // Attemp to reopen file //
        fopen_s(&file, filename, "r");
    }
    else if (file == NULL) {
        Log(LOG_ERROR|LOG_SYS,"Error opening file: %s", filename);
        return -1;
    }

	if (file != NULL)
	{
		char line[QUERY_SZ + SOURCE_SZ + 8];
		char delim[] = ":";
		char cDelim = '\'';
		char *strID,
			 *source,
			 *next_token,
			 *query;
		int comments = 1;
		unsigned int i = 0;

		while (fgets(line, sizeof(line), file) != NULL) { // read a line //
			if (line[0] == cDelim || strlen(line) < 1) {
				comments++;
                continue;
			}
            else {
                size_t len;
                int blank = 1;
                char *ptr = line;

                len = strlen(line);
                for (i=0; i<len; i++) {
                    if (*ptr != ' ') {
                        blank = 0;
                        break;
                    }
                    ptr++;
                }

                if (blank) continue;
            }
            
            if (_strnicmp(line, "XPath", 5) == 0) {
                XPathList* li = CheckXPath(*xpath_queries, line, delim);
                if (li != *xpath_queries) {
                    XPATH_COUNT++;
                    *xpath_queries = li;
                }
            }
			else {
				source = strtok_s(line, delim, &next_token);
				strID = strtok_s(NULL, delim, &next_token);
				query = strtok_s(NULL, delim, &next_token);
				if (source == NULL || strID == NULL) {
					Log(LOG_ERROR,"File format incorrect: %s line: %i", filename, i + comments);
					Log(LOG_ERROR,"Format should be [EventSource:[EventID|XPath:<Query>]] w/o brackets.");
                    fclose(file);
					return -1;
				}

				// Stop at MAX lines //
				if (i < MAX_IGNORED_EVENTS) {
					if (strID[0] == '*') {
						ignore_list[i].wild = TRUE;
						ignore_list[i].id = 0;
					}
					else {
						ignore_list[i].wild = FALSE;
						ignore_list[i].id = atoi(strID); // Enter id into array //
					}
					if (query != NULL){
						ignore_list[i].query = (char*)malloc(QUERY_SZ);
						strncpy_s(ignore_list[i].query, QUERY_SZ, source, _TRUNCATE);
					}
					// Enter source into array //
					if (source[0] == '*'){
						ignore_list[i].wildSource = TRUE;
					}
					else {
						strncpy_s(ignore_list[i].source, sizeof(ignore_list[i].source), source, _TRUNCATE);
					}

					if(LogInteractive)
						printf("IgnoredEvents[%i].id=%i \tIgnoredEvents[%i].source=%s\tIgnoredEvents[%i].query=%s\n", i, ignore_list[i].id, i, ignore_list[i].source, i, ignore_list[i].query);
				} else {
					// Notify if there are too many lines //
					Log(LOG_ERROR,"Config file too large. Max size is %i lines. Truncating...", MAX_IGNORED_EVENTS);
					break;
				}
                i++;
			}
		}
		fclose (file);
		IGNORED_LINES = i;

        if (LogInteractive)
        {
            BOOL winEvents = CheckForWindowsEvents();
            
            Log(LOG_INFO, "%s Filters: %i",
                winEvents ? "XPath" : (SyslogIncludeOnly ? "Include" : "Ignore"),
                winEvents ? XPATH_COUNT : i
            );
        }
	}

	// Can't run as IncludeOnly with no results set to include //
	if (SyslogIncludeOnly && IGNORED_LINES == 0)
	{
		Log(LOG_ERROR,"You cannot set the IncludeOnly flag and not specify any events to include!");
		return -1;
	}

	// Success //
	return 0;
}

// Check and add XPath to list //
XPathList* CheckXPath(XPathList * xpathList, char * line, char * delim)
{
    char *source = (char*)malloc(SOURCE_SZ);
    char *xpath = (char*)malloc(QUERY_SZ);
	char *plugin = (char*)malloc(PLUGIN_SZ);
	char *next_token;
	char *temp;

    strtok_s(line, delim, &next_token); // Remove 'XPath' from string

	temp = strtok_s(NULL, delim, &next_token);
	if (temp != NULL){
		if (_strcmpi(temp, "default") != 0){
			Log(LOG_INFO, "Plugin : %s", temp);
			strcpy_s(plugin, PLUGIN_SZ, temp);
		}
		else {
			free(plugin);
			plugin = NULL;
		}
	}

	strcpy_s(source, SOURCE_SZ, strtok_s(NULL, delim, &next_token)); // Get the source after the 'XPath'
	strcpy_s(xpath, QUERY_SZ, strtok_s(NULL, "\r\n", &next_token));  // Get the query
	
	if (source == NULL || xpath == NULL) {
        Log(LOG_ERROR, "Invalid XPath config: %s", line);
        return xpathList;
    }

    return AddXPath(xpathList, plugin, source, xpath);
}

XPathList* CreateXPath(char * plugin, char * source, char * query) {
    XPathList* newXPath = (XPathList*)malloc(sizeof(XPathList));
    if (newXPath != NULL){
		newXPath->plugin = plugin;
        newXPath->source = source;
        newXPath->query = query;
        newXPath->next = NULL;
    }
    
    if (LogInteractive)
        Log(LOG_INFO, "Creating %s", source);

    return newXPath;
}

XPathList* AddXPath(XPathList* xpathList, char * plugin, char * source, char * query) {
    XPathList* newXPath = CreateXPath(plugin, source, query);
    if (newXPath != NULL) {
        newXPath->next = xpathList;
    }

    return newXPath;
}

void DeleteXPath(XPathList* oldXPath) {
    if (oldXPath->next != NULL) {
        DeleteXPath(oldXPath->next);
    }

    if (LogInteractive)
        Log(LOG_INFO, "Deleting %s", oldXPath->source);
    
    if (oldXPath->source != NULL)
        free(oldXPath->source);

    if (oldXPath->query != NULL)
		free(oldXPath->query);

	if (oldXPath->plugin != NULL)
		free(oldXPath->plugin);

    free(oldXPath);
}

// Check Syslog Status Interval //
int CheckSyslogInterval(char * interval)
{
	DWORD minutes;

	// Convert interval to integer //
	minutes = atoi(interval);

	// Check for valid number //
	if (minutes < 0 || minutes > 65535) {
		Log(LOG_ERROR, "Bad interval: %s \nMust be between 0 and 65,535 minutes", interval);
		return 1;
	}

	// Store new value //
	SyslogStatusInterval = minutes;

	// Success //
	return 0;
}

// Check for DHCP flag //
int CheckSyslogQueryDhcp(char * arg)
{
	DWORD value;

	// Try converting to integer //
	value = atoi(arg);

	// Check for valid number //
	if (value < 0 || value > 0xffff) {
		Log(LOG_ERROR, "Invalid boolean value: %s", arg);
		return 1;
	}

	// Store new value //
	SyslogQueryDhcp = value ? TRUE : FALSE;

	// Success //
	return 0;
}

// Check for IncludeOnly flag //
int CheckSyslogIncludeOnly()
{
	// Store new value //
	SyslogIncludeOnly = TRUE;

	// Success //
	return 0;
}

int CheckSyslogTag(char * arg)
{
	if(strlen(arg) > sizeof(SyslogTag)-1) {
		Log(LOG_ERROR, "Syslog tag too long: \"%s\"", arg);
		return 1;
	}
	
	SyslogIncludeTag = TRUE;
	strncpy_s(SyslogTag, sizeof(SyslogTag), arg, _TRUNCATE);

	return 0;
}

// Check for new Crimson Log Service //
int CheckForWindowsEvents()
{
	HKEY hkey = NULL;
    BOOL winEvents = FALSE;

	// Check if the new Windows Events Service is in use //
	// If so we will use the new API's to sift through events //
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Channels\\ForwardedEvents", 0, KEY_READ, &hkey) != ERROR_SUCCESS)
		winEvents = FALSE;
	else
		winEvents = TRUE;
		
	if (hkey)
		RegCloseKey(hkey);

	// A level of 1 (Critical) is not valid in this process prior //
	// to the new Windows Events. Set level to 2 (Error) //
	if (winEvents == FALSE && SyslogLogLevel == 1)
		SyslogLogLevel = 2;

    return winEvents;
}