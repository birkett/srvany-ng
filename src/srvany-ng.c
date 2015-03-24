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

#define MAX_DATA_LENGTH 8192
#define MAX_KEY_LENGTH MAX_PATH
#define SERVICE_NAME TEXT("srvany-ng")

SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
PROCESS_INFORMATION   g_Process;

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
	while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(1000);
	}

	return ERROR_SUCCESS;
}

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
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
		SetEvent(g_ServiceStopEvent); //Kill the worker thread
		TerminateProcess(g_Process.hProcess, 0); //Kill the target process
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
}

void WINAPI ServiceMain(DWORD argc, TCHAR *argv[])
{
	Sleep(10000);

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

	TCHAR keyPath[MAX_KEY_LENGTH];
	wsprintf(keyPath, TEXT("%s%s%s"), TEXT("SYSTEM\\CurrentControlSet\\Services\\"), argv[0], TEXT("\\Parameters\\"));

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &openedKey) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("Faileed to open service parameters key\n"));
		ServiceSetState(0, SERVICE_STOPPED, 0);
		return;
	}

	TCHAR applicationString[MAX_DATA_LENGTH] = TEXT("");
	if (RegQueryValueEx(openedKey, TEXT("Application"), NULL, NULL, (LPBYTE)applicationString, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("Failed to open Application value\n"));
		ServiceSetState(0, SERVICE_STOPPED, 0);
		return;
	}

	TCHAR applicationParameters[MAX_DATA_LENGTH] = TEXT("");
	if (RegQueryValueEx(openedKey, TEXT("AppParameters"), NULL, NULL, (LPBYTE)applicationParameters, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppParameters key not found. Non fatal.\n"));
	}

	LPWCH applicationEnvironment = GetEnvironmentStrings();
	if (RegQueryValueEx(openedKey, TEXT("AppEnvironment"), NULL, NULL, (LPBYTE)applicationEnvironment, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppEnvironment key not found. Non fatal.\n"));
	}

	TCHAR applicationDirectory[MAX_DATA_LENGTH] = TEXT("");
	GetCurrentDirectory(MAX_DATA_LENGTH, applicationDirectory); //Default to the current dir when not specified in the registry
	if (RegQueryValueEx(openedKey, TEXT("AppDirectory"), NULL, NULL, (LPBYTE)applicationDirectory, &cbData) != ERROR_SUCCESS)
	{
		OutputDebugString(TEXT("AppDirectory key not found. Non fatal.\n"));
	}

	SetCurrentDirectory(applicationDirectory); //Set to either the current, or value in the registry

	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.wShowWindow = 0;
	startupInfo.lpReserved = NULL;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = NULL;

	if (CreateProcess(NULL, applicationString, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, applicationEnvironment, applicationDirectory, &startupInfo, &g_Process))
	{
		ServiceSetState(SERVICE_ACCEPT_STOP, SERVICE_RUNNING, 0);
		HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
		WaitForSingleObject(hThread, INFINITE); //Wait here for a stop signal
		CloseHandle(g_ServiceStopEvent);
	}

	ServiceSetState(0, SERVICE_STOPPED, 0);
}


int main(int argc, TCHAR *argv[])
{
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
}