/*  This DHCP Code is provided by Damien Mascre of the University of Paris */

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
#include "dhcp.h"
#include "log.h"
#include "syslog.h"


LPWSTR doMultiByteToWideChar(char *pszStringToConvert) {

	LPWSTR pszConverted = NULL;
	DWORD ccb;

	if( pszStringToConvert ) {

		ccb = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pszStringToConvert, -1, NULL, 0);
		if( ccb ) {
		
			pszConverted = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, ccb * sizeof(*pszConverted));
			if( pszConverted ) {

				ccb = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pszStringToConvert, -1, pszConverted, ccb);
				if( ccb ) {
					return pszConverted;
				}
			}
		}
	}

	return pszConverted;
}

int DHCPQuery(void) {

	int result = 1;
	PIP_ADAPTER_INFO pAdapterInfo, pAdapter;
	LPWSTR pszAdapterName;
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	DWORD dwDHCPVersion;
	DWORD dwError, dwSize;
	char szOption[1000];
	DHCPCAPI_PARAMS dpParams;		
	DHCPCAPI_PARAMS_ARRAY dpReqArray;
	DHCPCAPI_PARAMS_ARRAY dpSendArray;

	if( DhcpCApiInitialize(&dwDHCPVersion) != ERROR_SUCCESS ) {
		Log(LOG_ERROR|LOG_SYS, "DhcpCApiInitialize failed.");
		return 1;
	}

	pAdapterInfo = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulOutBufLen);
	if( pAdapterInfo == NULL ) {
		return 1;
	}

	dwError = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if( dwError == ERROR_BUFFER_OVERFLOW ) {
		HeapFree(GetProcessHeap(), 0, pAdapterInfo);

		pAdapterInfo = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulOutBufLen);
		if( pAdapterInfo == NULL ) {
			return 1;
		}

		dwError = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	}

	if( dwError == NO_ERROR) {
	
		/* on cherche l'interface réseau à utiliser... */
		pAdapter = pAdapterInfo;
		while( pAdapter) {

			if( (pAdapter->Type == MIB_IF_TYPE_ETHERNET) && pAdapter->DhcpEnabled ) {

				pszAdapterName = doMultiByteToWideChar(pAdapter->AdapterName);

				dpParams.Flags = 0;
				dpParams.OptionId = OPTION_LOG_SERVERS; // that's what i am talking about !
				dpParams.IsVendor = FALSE;
				dpParams.Data = NULL;
				dpParams.nBytesData = 0;

				dpReqArray.nParams = 1;
				dpReqArray.Params = &dpParams;
				
				dpSendArray.nParams = 0;
				dpSendArray.Params = NULL;

				dwSize = sizeof(szOption);

				if( dpParams.OptionId > 67 ) {

					/*
					 * microsoft windows has a big problem with non-standard dhcp option
					 * which need us to "install" a "persistent dhcp request" in order
					 * to be able to retrieve it...
					 *
					 * I have seen some windows still not being able to get us
					 * the standard options without using a persistent request,
					 * so activating this branch of code will do the trick,
					 * just notice that in order to work, the system will only
					 * work after the second boot, because as said in msdn docs,
					 * the persistent request is only done at boot time, so the
					 * first registers the request, the second boot does it.
					 *
					 * In the sake of being completely documented, knowing where
					 * to look in case things go wrong :
					 *
					 * HKLM\System\CurrentControlSet\Services\Dhcp\Parameters :
					 * the guid keys are the guid of the network adapters,
					 * and the values are simply the dhcp packets, so look into
					 * those values, and you will read the options as passed
					 * by the dhcp server (you will recognize the options windows
					 * say it knows nothing about.. but here they are).
					 *
					 * HKLM\System\CurrentControlSet\Services\Dhcp\Parameters\Options :
					 * lists the "option" windows know about, kind of factory defaults.
					 * Unusable for us, but it is here that you will see new keys
					 * appear when you activate the "persistent request" mecanism.
					 */

					/* 
					 * we still try unregistering the request, since it leads to
					 * a "parameter error" if the request is already set when calling
					 * the next function, which we choose to force each time, because
					 * there is no way of knowing if the request is set and registered, or not.
					 */
					DhcpUndoRequestParams(0, NULL, pszAdapterName, L"EventToSyslogDhcpOption");

					dwError = DhcpRequestParams(
						DHCPCAPI_REQUEST_SYNCHRONOUS | DHCPCAPI_REQUEST_PERSISTENT,
						NULL,
						pszAdapterName,
						NULL,
						dpSendArray,
						dpReqArray,
						(PBYTE)szOption,
						&dwSize,
						L"EventToSyslogDhcpOption"
					);

				} else {

					dwError = DhcpRequestParams(
						DHCPCAPI_REQUEST_SYNCHRONOUS,
						NULL,
						pszAdapterName,
						NULL,
						dpSendArray,
						dpReqArray,
						(PBYTE)szOption,
						&dwSize,
						NULL
					);
				}

				if( (ERROR_SUCCESS == dwError) && (dpParams.nBytesData == 4) ) {

					/* the option is sent as 4 packed bytes */
					sprintf_s(SyslogLogHostDhcp, sizeof(SyslogLogHostDhcp), "%d.%d.%d.%d", dpParams.Data[0], dpParams.Data[1], dpParams.Data[2], dpParams.Data[3]);
					Log(LOG_INFO|LOG_SYS, "Got DHCP Syslog Server option : %s", SyslogLogHostDhcp);

					/* success */
					result = 0;

				} else {
					Log(LOG_ERROR|LOG_SYS, "Unable to retrieve dhcp option : error %08X.", dwError);
				}

				HeapFree(GetProcessHeap(), 0, pszAdapterName);
			}

			pAdapter = pAdapter->Next;
		}
	}

	if( pAdapterInfo ) {
		HeapFree(GetProcessHeap(), 0, pAdapterInfo);
	}

	DhcpCApiCleanup();
	return result;
}
