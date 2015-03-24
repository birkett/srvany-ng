/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Anthony Birkett
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <Windows.h>

#define MAX_DATA_LENGTH 8192				//Max lenght of a registry value
#define MAX_KEY_LENGTH	MAX_PATH			//Max length of a registry path
#define SERVICE_NAME	TEXT("srvany-ng")	//Name to reference this service

SERVICE_STATUS_HANDLE	g_StatusHandle		= NULL;
HANDLE					g_ServiceStopEvent	= INVALID_HANDLE_VALUE;
PROCESS_INFORMATION		g_Process			= { 0 };


/*
 * Worker thread for the service. Currently serves no real purpse but to keep
 *  the service started. 
 *
 * Eventually, this should check if the target process has exited, and stop
 *  or restart the service accordingly.
 */
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);

	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(1000);
	}

	return ERROR_SUCCESS;
}//end ServiceWorkerThread()


/*
 * Set the current state of the service.
 */
void ServiceSetState(DWORD acceptedControls, DWORD newState, DWORD exitCode)
{
	SERVICE_STATUS serviceStatus;
	ZeroMemory(&serviceStatus, sizeof(SERVICE_STATUS));
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwControlsAccepted = acceptedControls;
	serviceStatus.dwCurrentState = newState;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwWaitHint = 0;
	serviceStatus.dwWin32ExitCode = exitCode;

	if (SetServiceStatus(g_StatusHandle, &serviceStatus) == FALSE)
	{
		OutputDebugString(TEXT("SetServiceStatus failed\n"));
	}
}//end ServiceSetState()


/*
 * Handle service control requests, like STOP, PAUSE and CONTINUE.
 */
void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
		SetEvent(g_ServiceStopEvent); //Kill the worker thread.
		TerminateProcess(g_Process.hProcess, 0); //Kill the target process.
		ServiceSetState(0, SERVICE_STOPPED, 0);
		break;

	case SERVICE_CONTROL_PAUSE:
		ServiceSetState(0, SERVICE_PAUSED, 0);
		break;

	case SERVICE_CONTROL_CONTINUE:
		ServiceSetState(0, SERVICE_RUNNING, 0);
		break;

	default:
		break;
	}
}//end ServiceCtrlHandler()


/*
 * Main entry point for the service. Acts in a similar fasion to main().
 *
 * NOTE: argv[0] will always contain the service name when invoked by the SCM.
 */
void WINAPI ServiceMain(DWORD argc, TCHAR *argv[])
{
	UNREFERENCED_PARAMETER(argc);

//Pause on start for Debug builds. Gives some time to manually attach a debugger.
#ifdef _DEBUG
	Sleep(10000);
#endif

	g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (g_StatusHandle == NULL)
	{
		ServiceSetState(0, SERVICE_STOPPED, GetLastError());
		return;
	}

	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		ServiceSetState(0, SERVICE_STOPPED, GetLastError());
		return;
	}

	HKEY openedKey;
	DWORD cbData = MAX_DATA_LENGTH;

	//Open the registry key for this service.
	TCHAR keyPath[MAX_KEY_LENGTH];
	wsprintf(keyPath, TEXT("%s%s%s"), TEXT("SYSTEM\\CurrentControlSet\\Services\\"), argv[0], TEXT("\\Parameters\\"));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &openedKey) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("Faileed to open service parameters key\n"));
		ServiceSetState(0, SERVICE_STOPPED, 0);
		return;
	}

	//Get the target application path from the Parameters key.
	TCHAR applicationString[MAX_DATA_LENGTH] = TEXT("");
	if (RegQueryValueEx(openedKey, TEXT("Application"), NULL, NULL, (LPBYTE)applicationString, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("Failed to open Application value\n"));
		ServiceSetState(0, SERVICE_STOPPED, 0);
		return;
	}

	//Get the target application parameters from the Parameters key.
	TCHAR applicationParameters[MAX_DATA_LENGTH] = TEXT("");
	if (RegQueryValueEx(openedKey, TEXT("AppParameters"), NULL, NULL, (LPBYTE)applicationParameters, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppParameters key not found. Non fatal.\n"));
	}

	//Get the target application environment from the Parameters key.
	LPWCH applicationEnvironment = GetEnvironmentStrings(); //Default to the current environment.
	if (RegQueryValueEx(openedKey, TEXT("AppEnvironment"), NULL, NULL, (LPBYTE)applicationEnvironment, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppEnvironment key not found. Non fatal.\n"));
	}

	//Get the target application directory from the Parameters key.
	TCHAR applicationDirectory[MAX_DATA_LENGTH] = TEXT("");
	GetCurrentDirectory(MAX_DATA_LENGTH, applicationDirectory); //Default to the current dir when not specified in the registry.
	if (RegQueryValueEx(openedKey, TEXT("AppDirectory"), NULL, NULL, (LPBYTE)applicationDirectory, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppDirectory key not found. Non fatal.\n"));
	}
	SetCurrentDirectory(applicationDirectory); //Set to either the current, or value in the registry.

	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.wShowWindow = 0;
	startupInfo.lpReserved = NULL;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = NULL;

	//Try to launch the target application.
	if (CreateProcess(NULL, applicationString, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, applicationEnvironment, applicationDirectory, &startupInfo, &g_Process))
	{
		ServiceSetState(SERVICE_ACCEPT_STOP, SERVICE_RUNNING, 0);
		HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
		WaitForSingleObject(hThread, INFINITE); //Wait here for a stop signal.
		CloseHandle(g_ServiceStopEvent);
	}

	ServiceSetState(0, SERVICE_STOPPED, 0);
}//end ServiceMain()


/*
 * Main entry point for the application.
 *
 * NOTE: The SCM calls this, just like any other application.
 */
int main(int argc, TCHAR *argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		return GetLastError();
	}

	return 0;
}//end main()
