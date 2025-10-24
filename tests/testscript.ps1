<#
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
#>

<#
 * Install a test service, targeting the test app.
 *  This is used to make sure srvany-ng is correctly passing the values defined
 *  in the Parameters registry key.
#>

# Script has to run elevated, for Service install / removal and registry write access.
If (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    Write-Warning "Run this script as an Administrator!"
    Break
}

# Binary paths.
$binaryPath = $PSScriptRoot + "\..\" + "msvc\build\bin\srvany-ng\Debug Unicode_x64\srvany-ng_du64.exe"
$testAppPath = $PSScriptRoot + "\..\" + "msvc\build\bin\testapp\Debug Unicode_x64\testapp_du64.exe"


# Test data, the output directory (AppDirectory), parameters (AppParameters) and EnvVars (AppEnvironment).
$serviceName = "srvany-testapp"
$testAppOutput = $PSScriptRoot + "\..\" + "tests\"
$testAppParams = "-someparam -another param2"
$testAppEnv = "SOMEENVAR=SOMEVALUE", "ANOTHERVAR=VALUE2"
$overrideEnvironment = 1

# Create the service.
Write-Host "Creating service $serviceName"
New-Service -Name $serviceName -BinaryPathName $binaryPath

# Create the registry keys and populate test data.
Write-Host "Creating registry keys"
New-Item -Path HKLM:\System\CurrentControlSet\Services\$serviceName -Name "Parameters" –Force | out-null
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\$serviceName\Parameters -Name "Application" -PropertyType String -Value $testAppPath | out-null
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\$serviceName\Parameters -Name "AppDirectory" -PropertyType String -Value $testAppOutput | out-null
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\$serviceName\Parameters -Name "AppParameters" -PropertyType String -Value $testAppParams | out-null
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\$serviceName\Parameters -Name "AppEnvironment" -PropertyType MultiString -Value $testAppEnv | out-null
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\$serviceName\Parameters -Name "OverrideEnvironment" -PropertyType Dword -Value $overrideEnvironment | out-null

# Start the service.
Write-Host "Starting service $serviceName"
Start-Service $serviceName

# Stop the service if testapp hung for some reason. (When testapp closes, the service should auto stop).
Write-Host "Stopping service $serviceName"
Stop-Service $serviceName

# No cmdlet available for this, call sc to delete the service (will clean up the whole registry key).
Write-Host "Deleting service $serviceName"
sc.exe delete $serviceName
