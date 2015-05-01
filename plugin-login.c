
#include "main.h"
#include <malloc.h>
#include <wchar.h>
#include <winevt.h>
#include <winmeta.h>
#include "log.h"
#include "service.h"
#include "syslog.h"
#include "winevent.h"
#include "check.h"
#include "plugin-login.h"

#pragma comment(lib, "delayimp.lib") /* Prevents winevt from loading unless necessary */
#pragma comment(lib, "wevtapi.lib")	 /* New Windows Events logging library for Vista and beyond */

WCHAR * pluginLogin(EVT_HANDLE hEvent)
{
	PEVT_VARIANT eventInfo = NULL;
	eventInfo = GetLoginInfo(hEvent);
	static WCHAR message[SYSLOG_DEF_SZ];
	static WCHAR * wsGuid;
	

	if (!eventInfo) {
		return NULL;
	}

	if (eventInfo[0].GuidVal == NULL || EndsWith(eventInfo[1].StringVal, L"$") || StartsWith(eventInfo[1].StringVal, L"MSOL_")){
		free(eventInfo);
		return NULL;
	}
	// Print the values from the System section of the element.
	wsGuid = GuidToString(eventInfo[0].GuidVal);

	_snwprintf_s(message, COUNT_OF(message), _TRUNCATE, L"%s:%s:%s:%s",
		wsGuid,
		 eventInfo[1].StringVal, eventInfo[2].StringVal, eventInfo[3].StringVal
		);

	free(eventInfo);
	 //(LPWSTR)eventInfo[0].StringVal;

	return message;

}


/* Get specific values from an event */
PEVT_VARIANT GetLoginInfo(EVT_HANDLE hEvent)
{
	EVT_HANDLE hContext = NULL;
	PEVT_VARIANT pRenderedEvents = NULL;
	LPWSTR ppValues[] = {
		L"Event/EventData/Data[@Name='LogonGuid']",
		L"Event/EventData/Data[@Name='TargetUserName']",
		L"Event/EventData/Data[@Name='WorkstationName']",
		L"Event/EventData/Data[@Name='IpAddress']" };
	DWORD count = COUNT_OF(ppValues);
	DWORD dwReturned = 0;
	DWORD dwBufferSize = (256 * sizeof(LPWSTR)*count);
	DWORD dwValuesCount = 0;
	DWORD status = 0;

	/* Create the context to use for EvtRender */
	hContext = EvtCreateRenderContext(count, (LPCWSTR*)ppValues, EvtRenderContextValues);
	if (NULL == hContext) {
		Log(LOG_ERROR | LOG_SYS, "EvtCreateRenderContext failed");
		goto cleanup;
	}

	pRenderedEvents = (PEVT_VARIANT)malloc(dwBufferSize);
	/* Use EvtRender to capture the Publisher name from the Event */
	/* Log Errors to the event log if things go wrong */
	if (!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedEvents, &dwReturned, &dwValuesCount)) {
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			dwBufferSize = dwReturned;
			realloc(pRenderedEvents, dwBufferSize);
			if (!EvtRender(hContext, hEvent, EvtRenderEventValues, dwBufferSize, pRenderedEvents, &dwReturned, &dwValuesCount)) {
				if (LogInteractive)
					Log(LOG_ERROR | LOG_SYS, "Error Rendering Event");
				status = ERR_FAIL;
			}
		}
		else {
			status = ERR_FAIL;
			if (LogInteractive)
				Log(LOG_ERROR | LOG_SYS, "Error Rendering Event");
		}
	}

cleanup:
	if (hContext)
		EvtClose(hContext);

	if (status == ERR_FAIL)
		return NULL;
	else
		return pRenderedEvents;
}

WCHAR * GuidToString(GUID * guid)
{
	static WCHAR wsGuid[50];
	if (guid == NULL){
		_snwprintf_s(wsGuid, COUNT_OF(wsGuid), _TRUNCATE, L"00000000-0000-0000-0000-000000000000");
		return wsGuid;
	}

	_snwprintf_s(wsGuid, COUNT_OF(wsGuid), _TRUNCATE, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		guid->Data1,
		guid->Data2,
		guid->Data3,
		guid->Data4[0],
		guid->Data4[1],
		guid->Data4[2],
		guid->Data4[3],
		guid->Data4[4],
		guid->Data4[5],
		guid->Data4[6],
		guid->Data4[7]
		);

	return wsGuid;
}