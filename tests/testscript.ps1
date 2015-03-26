If (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
    Write-Warning "Run this script as an Administrator!"
    Break
}

$binaryPath = $PSScriptRoot + "\..\" + "msvc\build\srvany-ng\Debug_x64\srvany-ng_d64.exe"
$testAppPath = $PSScriptRoot + "\..\" + "msvc\build\testapp\Debug_x64\testapp.exe"
$testAppOutput = $PSScriptRoot + "\..\" + "tests\"
$testAppParams = "-someparam -another param2"
$testAppEnv = "SOMEENVAR=SOMEVALUE", "ANOTHERVAR=VALUE2"

New-Service -Name "srvany-testapp" -BinaryPathName $binaryPath

New-Item -Path HKLM:\System\CurrentControlSet\Services\srvany-testapp -Name "Parameters" –Force
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\srvany-testapp\Parameters -Name "Application" -PropertyType String -Value $testAppPath
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\srvany-testapp\Parameters -Name "AppDirectory" -PropertyType String -Value $testAppOutput
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\srvany-testapp\Parameters -Name "AppParameters" -PropertyType String -Value $testAppParams
New-ItemProperty -Path HKLM:\System\CurrentControlSet\Services\srvany-testapp\Parameters -Name "AppEnvironment" -PropertyType MultiString -Value $testAppEnv

Start-Service srvany-testapp

Stop-Service srvany-testapp

sc.exe delete srvany-testapp
