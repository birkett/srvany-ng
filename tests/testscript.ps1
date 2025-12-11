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

function Create-TestService {
    param(
        [Parameter(Mandatory=$true)]
        [string] $ServiceName,
        [Parameter(Mandatory=$true)]
        [string] $BinaryPath
    )

    # Create the service.
    Write-Host "Creating service $ServiceName"
    New-Service -Name $ServiceName -BinaryPathName $BinaryPath
}

function Set-TestServiceParameters {
    param(
        [Parameter(Mandatory=$true)]
        [string] $ServiceName,
        [Parameter(Mandatory=$true)]
        [string] $Application,
        [Parameter(Mandatory=$true)]
        [string] $AppDirectory,
        [Parameter(Mandatory=$true)]
        [string] $AppParameters,
        [string[]] $AppEnvironment,
        [Nullable[int]] $OverrideEnvironment
    )

    # Create the registry keys and populate test data.
    Write-Host "Creating registry keys"

    $regPath = "HKLM:\System\CurrentControlSet\Services\$ServiceName\Parameters"

    New-Item -Path $regPath -Force | out-null

    New-ItemProperty -Path $regPath -Name "Application" -PropertyType String -Value $Application | out-null
    New-ItemProperty -Path $regPath -Name "AppDirectory" -PropertyType String -Value $AppDirectory | out-null
    New-ItemProperty -Path $regPath -Name "AppParameters" -PropertyType String -Value $AppParameters | out-null

    if ($PSBoundParameters.ContainsKey('AppEnvironment') -and $null -ne $AppEnvironment) {
        New-ItemProperty -Path $regPath -Name "AppEnvironment" -PropertyType MultiString -Value $AppEnvironment | out-null
    }

    if ($PSBoundParameters.ContainsKey('OverrideEnvironment') -and $null -ne $OverrideEnvironment) {
        New-ItemProperty -Path $regPath -Name "OverrideEnvironment" -PropertyType Dword -Value $OverrideEnvironment | out-null
    }
}

function Invoke-ServiceLifecycle {
    param(
        [Parameter(Mandatory=$true)]
        [string] $ServiceName
    )

    # Start the service.
    Write-Host "Starting service $ServiceName"
    Start-Service $ServiceName

    # Stop the service if testapp hung for some reason. (When testapp closes, the service should auto stop).
    Write-Host "Stopping service $ServiceName"
    Stop-Service $ServiceName

    # No cmdlet available for this, call sc to delete the service (will clean up the whole registry key).
    Write-Host "Deleting service $ServiceName"
    sc.exe delete $ServiceName
}

# Script has to run elevated, for Service install / removal and registry write access.
If (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    Write-Warning "Run this script as an Administrator!"
    Break
}

# Test data
$testCases = @(
    @{
        Name = "No AppEnvironment - No OverrideEnvironment"
        AppEnvironment = $null
        OverrideEnvironment = $null
    },
    @{
        Name = "No AppEnvironment - OverrideEnvironment 0"
        AppEnvironment = $null
        OverrideEnvironment = 0
    },
    @{
        Name = "No AppEnvironment - OverrideEnvironment 1"
        AppEnvironment = $null
        OverrideEnvironment = 1
    },
    @{
        Name = "AppEnvironment Present - No OverrideEnvironment"
        AppEnvironment = @("SOMEENVAR=SOMEVALUE", "ANOTHERVAR=VALUE2")
        OverrideEnvironment = $null
    },
    @{
        Name = "AppEnvironment Present - OverrideEnvironment 0"
        AppEnvironment = @("SOMEENVAR=SOMEVALUE", "ANOTHERVAR=VALUE2")
        OverrideEnvironment = 0
    },
    @{
        Name = "AppEnvironment Present - OverrideEnvironment 1"
        AppEnvironment = @("SOMEENVAR=SOMEVALUE", "ANOTHERVAR=VALUE2")
        OverrideEnvironment = 1
    }
)

foreach ($case in $testCases) {
    # Binary paths.
    $binaryPath = $PSScriptRoot + "\..\" + "msvc\build\bin\srvany-ng\Debug Unicode_x64\srvany-ng_du64.exe"
    $testAppPath = $PSScriptRoot + "\..\" + "msvc\build\bin\testapp\Debug Unicode_x64\testapp_du64.exe"

    $slugifiedCaseName = $case.Name -replace '\s+', '-'
    $slugifiedCaseName = $slugifiedCaseName -replace '--', '-'

    $serviceName = "srvany-testapp-" + $slugifiedCaseName
    $testAppOutput  = $PSScriptRoot + "\..\" + "tests\"
    $testAppParams = "-someparam -another param2"

    Write-Host "---------------------------------------"
    Write-Host " RUNNING TEST CASE: $slugifiedCaseName"
    Write-Host "---------------------------------------"

    Create-TestService -ServiceName $serviceName -BinaryPath $binaryPath

    Set-TestServiceParameters -ServiceName $serviceName -Application $testAppPath -AppDirectory $testAppOutput -AppParameters $testAppParams -AppEnvironment $case.AppEnvironment -OverrideEnvironment $case.OverrideEnvironment

    Invoke-ServiceLifecycle -ServiceName $serviceName

    Move-Item -Force -Path ($testAppOutput + "output.txt") -Destination ($testAppOutput + $slugifiedCaseName + ".txt")
}
