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
		OutputDebugString(TEXT("SetServiceStatus failed"));
	}
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
		ServiceSetState(0, SERVICE_STOPPED, 0);

		SetEvent(g_ServiceStopEvent);

		TerminateProcess(g_Process.hProcess, 0);
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

	ServiceSetState(SERVICE_ACCEPT_STOP, SERVICE_RUNNING, 0);

	//if (CreateProcess())
	{
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