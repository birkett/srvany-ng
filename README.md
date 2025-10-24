# srvany-ng: Run any Windows application as a Service
A drop in, compatible, replacement for the useful "srvany.exe", found in the Windows Server Resource Kit.<br />
Written in C, for Windows XP and newer.


## Support Me
[![Donate Via Paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=CALMNQUWLZNYL)
<br /><br />
## Differences to the original
* Open Source
* 32 and 64 bit binary available
* Unicode (default) and Multibyte Character Set (MBCS) binaries available
* No OS/2 application support
* Service will stop if the launched application exits
* Added optional configuration to automatically restart the target application on exit
* Application environment variables will inherit the global environment by default, see Application Environment below
* Added optional configuration to allow terminating of sub processes spawned by the service

## Building
Solution is known to build with Visual Studio 2022, no additional dependencies are needed. The project uses the "v141_xp" toolset to ensure XP / 2003 support, and links to the MSVCRT statically.
<br /><br />

## Installing
Place srvany-ng in an accessible folder on your system.
Install it as a service from an Elevated (Administrator) Command Prompt:
```winbatch
sc create "MyServiceName" start= auto binPath= "C:\Path\To\srvany-ng.exe"
sc description MyServiceName "My services description"
```
Note the spaces between `start=`, `binPath=` and their parameters. This is intended.

Now, open the Registry editor (`regedit.exe`), and browse to:
`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\MyServiceName`

Create a new Key named "Parameters".
In the parameters key, create a new String value named "Application". The value should be the file path to the application you wish to run as a service.

#### Optional Parameters
| Value name            | Value type         | Description                                                                                       |
|-----------------------|--------------------|---------------------------------------------------------------------------------------------------|
| AppDirectory          | String value       | The starting directory for your application. Usually the same as the folder its executable is in. |
| AppParameters         | String value       | Command line arguments to pass to your application on startup.                                    |
| AppEnvironment        | Multi-String value | Environment variables to set for your application.                                                |
| RestartOnExit         | DWORD value        | If set to 1, and the application exits, srvany-ng will automatically restart it.                  |
| OverrideEnvironment   | DWORD value        | Default 0, if set to 1 will enable the original srvany environment behaviour described below      |
| TerminateSubProcesses | DWORD value        | If set to 1, will use a JobObject to terminate sub processes spawned by the service               |

## Application Environment
The original `srvany` will overwrite the global environment variables if `AppEnvironment` is configured. If `AppEnvironment` is not configured, the default global environment would be
passed to the running application.

`srvany-ng` changes this behaviour, to always pass the global environment variables to the running application, in addition to anything configured in `AppEnvironment`.

This behaviour can be restored to the original `srvany` behaviour by setting `DWORD OverrideEnvironment = 1` in the registry parameters.

## Further Reading
Microsoft support article describing the use of the original srvany.exe: https://support.microsoft.com/en-us/kb/137890
<br />
