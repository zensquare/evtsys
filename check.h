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

/* CONSTANTS */
#define MIN_LOG_LEVEL 0
#define MAX_LOG_LEVEL 4

/* External Variables */
extern int IGNORED_LINES;
extern int XPATH_COUNT;

/* Ignored Events Structure */
struct EVENT_LIST {
	char source[SOURCE_SZ];
	WCHAR wsource[SOURCE_SZ];
	char * query;
	BOOL wild;
	BOOL wildSource;
	int id;
};

/* XPath Query Structure */
struct XPATH_LIST {
	char * plugin;
    char * source;
    char * query;
    struct XPATH_LIST* next;
};

/* Prototypes */
XPathList* AddXPath(XPathList* xpathList, char * plugin, char * source, char * query);
XPathList* CreateXPath(char * plugin, char * source, char * query);
void       DeleteXPath(XPathList* oldXPath);
char*	   getConfigPath();