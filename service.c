/*
  Copyright (c) 1998-2007, Purdue University
  All rights reserved.

  Redistribution and use in source and binary forms are permitted provided
  that:

  (1) source distributions retain this entire copyright notice and comment,
      and
  (2) distributions including binaries display the following acknowledgement:

         "This product includes software developed by Purdue University & Rochester Institute of Technology."

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

/* Service */

/* Effect main loop */
BOOL ServiceIsRunning = TRUE;

/* Forward */
static void WINAPI ServiceMain(DWORD argc, LPTSTR * argv);

/* Service dispatch table */
static SERVICE_TABLE_ENTRY ServiceDispatchTable[] = {
	{ "EvtSys", ServiceMain },
	{ NULL, NULL }
};

/* Service handle */
static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

/* Install a service */
int ServiceInstall()
{
	SC_HANDLE service_manager;
	SC_HANDLE new_service;

	/* Get a connection to the service manager */
	service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (service_manager == NULL) {
		Log(LOG_ERROR|LOG_SYS, "Cannot initialize access to the service manager");
		return 1;
	}

	/* Create a new service */
	new_service = CreateService(service_manager, "EvtSys", "Eventlog to Syslog", SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, ProgramExePath, NULL, NULL, "eventlog\0", NULL, NULL);
	if (new_service == NULL)
		Log(LOG_ERROR|LOG_SYS, "Cannot create service");
	else
		CloseServiceHandle(new_service);

	/* Close the connection to the manager */
	CloseServiceHandle(service_manager);

	/* Return status */
	return new_service == NULL;
}

/* Remove a service */
int ServiceRemove()
{
	int status = 1;
	SC_HANDLE service_manager;
	SC_HANDLE service_handle;
	SERVICE_STATUS query_status;

	/* Get a connection to the service manager */
	service_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (service_manager == NULL) {
		Log(LOG_ERROR|LOG_SYS, "Cannot initialize access to the service manager");
		return 1;
	}

	/* Connect to service */
	service_handle = OpenService(service_manager, "EvtSys", SERVICE_ALL_ACCESS);
	if (service_handle == NULL) {
		Log(LOG_ERROR|LOG_SYS, "Cannot open service");
	} else {

		/* Check that service is stopped */
		if (QueryServiceStatus(service_handle, &query_status) && query_status.dwCurrentState != SERVICE_STOPPED) {
			Log(LOG_ERROR, "Service currently in operation - Stop service and rerun the uninstall.");
		} else {

			/* Delete service */
			if (DeleteService(service_handle) == FALSE)
				Log(LOG_ERROR|LOG_SYS, "Cannot delete service");
			else
				status = 0;
		}

		/* Close connection to service */
		CloseServiceHandle(service_handle);
	}

	/* Close connection to service manager */
	CloseServiceHandle(service_manager);

	/* Return status */
	return status;
}

/* Process service changes */
static void WINAPI ServiceChange(DWORD code)
{
	/* Check for stop request
	(we may want to implement "pause" and "continue" later on) */
	if (code == SERVICE_CONTROL_STOP) {

		/* Terminate loop */
		ServiceIsRunning = FALSE;

		/* Wait until service stops */
		while (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
			Sleep(500);
	}
}

/* Process main loop */
static void WINAPI ServiceMain(DWORD argc, LPTSTR * argv)
{
	/* Register a control function to the service manager */
	ServiceStatusHandle = RegisterServiceCtrlHandler("EvtSys", ServiceChange);
	if (ServiceStatusHandle == 0) {
		Log(LOG_ERROR|LOG_SYS, "Cannot register a control handler for service");
		return;
	}

	/* Send start report */
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0; 
	if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE) {
		Log(LOG_ERROR|LOG_SYS, "Cannot send start service status update");
		return;
	}

	/* Process main loop */
	MainLoop();

	/* Send stop message */
	ServiceStatus.dwCurrentState = SERVICE_STOPPED;

	/* Report status */
	if (SetServiceStatus(ServiceStatusHandle, &ServiceStatus) == FALSE) {
		Log(LOG_ERROR|LOG_SYS, "Cannot send change service status update");
		return;
	}
}

/* Start service dispatcher */
DWORD WINAPI ServiceStart()
{
	/* Start service dispatch */
	if (StartServiceCtrlDispatcher(ServiceDispatchTable) == FALSE) {
		Log(LOG_ERROR|LOG_SYS, "Cannot start service dispatcher");
		return 1;
	}
	return 0;
}
