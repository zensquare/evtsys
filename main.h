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

/* Basic include files */
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <dhcpcsdk.h>
#include <lm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* Macros */
#define COUNT_OF(x)	(sizeof(x)/sizeof(*x))

/* Constants */
#define ERRMSG_SZ	256
#define MAX_IGNORED_EVENTS	256
#define HOSTNAME_SZ 64

#define QUERY_SZ	1024
#define QUERY_LIST_SZ  524288
#define ERR_FAIL	-1
#define ERR_CONTINUE 3
#define SOURCE_SZ	128
#define PLUGIN_SZ	32

/* Compatibility */
#define in_addr_t	unsigned long

typedef struct EVENT_LIST EventList;
typedef struct XPATH_LIST XPathList;

extern int  IGNORED_LINES;
extern EventList IgnoredEvents[MAX_IGNORED_EVENTS] ;
extern XPathList * XPathQueries;
extern BOOL ProgramUseIPAddress;
extern char ProgramHostName[256];
extern char ProgramExePath[MAX_PATH];

/* Prototypes */
int     CheckForWindowsEvents();
int     CheckSyslogFacility(char * facility);
int     CheckSyslogIgnoreFile(EventList * ignore_list, XPathList ** xpath_queries, char * filename);
int	    CheckSyslogInterval(char * interval);
int     CheckSyslogLogHost(char * loghostarg);
int     CheckSyslogPort(char * port);
int     CheckSyslogQueryDhcp(char * value);
int     CheckSyslogLogLevel(char * level);
int     CheckSyslogIncludeOnly();
int		CheckSyslogTag(char * arg);
XPathList* CheckXPath(XPathList * xpath_queries, char * line, char * delim);
char*   CollapseExpandMessage(char * message);
WCHAR*  CollapseExpandMessageW(WCHAR * message);
int	    ConnectSocket(char * loghost, unsigned short port, int ID);
int     ConvertLogHostToIp(char * loghost, char ** ipstr);
DWORD   CreateConfigFile(char * filename);
boolean    CreateQueryString(WCHAR * pQuery, XPathList * xpathQueries);
int     EventlogCreate(char * name);
char*   EventlogNext(EventList ignore_list[MAX_IGNORED_EVENTS], int log, int * level);
void    EventlogsClose(void);
int     EventlogsOpen(void);
char*   FormatLibraryMessage(char * message_file, DWORD event_id, char ** string_array);
void    GetError(DWORD err_num, char * message, int len);
void    GetErrorW(DWORD err_num, WCHAR * message, int len);
int     GetOpt(int nargc, char ** nargv, char * ostr);
char*   GetTimeStamp(void);
char*   GetUsername(SID * sid);
char*   GetWinEvent(char * log, int recNum, int event_id);
BOOL    IgnoreSyslogEvent(EventList * ignore_list, const char * E_SOURCE, int E_ID);
int     LogStart(void);
void    LogStop(void);
void    Log(int level, char * message, ...);
char*   LookupMessageFile(char * logtype, char * source, DWORD eventID);
int     MainLoop(void);
int     RegistryGather(void);
int     RegistryInstall(void);
int     RegistryRead(void);
int     RegistryUninstall(void);
int     RegistryUpdate();
int     ServiceInstall(void);
int     ServiceRemove(void);
DWORD   WINAPI ServiceStart(void);
BOOL    WINAPI ShutdownConsole(DWORD dwCtrlType);
void    SyslogClose(void);
int     SyslogOpen(void);
int     SyslogSend(char * message, int level);
int     SyslogSendW(WCHAR * message, int level);
void    WSockClose(void);
int     WSockOpen(char * loghost, unsigned short port, int ID);
int     WSockSend(char * message);
int     WSockStart(void);
void    WSockStop(void);
int		EndsWith(const LPWSTR str, const LPWSTR suffix);
int		StartsWith(const LPWSTR str, const LPWSTR suffix);