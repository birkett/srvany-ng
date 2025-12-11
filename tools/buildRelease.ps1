<#
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Anthony Birkett
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
 * Automate the creation of a release archive.
 *  Checks the build version number, then takes the build artefacts,
 *  copies them over to a release structure, and creates the final
 *  release archive.
#>

param(
    [Parameter(Mandatory=$true)]
    [string] $Version
)

function Check-RcFileVersion {
    param(
        [Parameter(Mandatory=$true)]
        [string] $RequestedVersion
    )

    $rcFile = $PSScriptRoot + "\..\" + "src\srvany-ng.rc"

    if (!(Test-Path $rcFile)) {
        Write-Error "RC file not found: $rcFile"
        exit 1
    }

    $content = Get-Content $rcFile -Raw

    $szPattern = '#define\s+BASEVERSION_SZ\s+"([^"]+)"'
    $szMatch = [regex]::Match($content, $szPattern)

    if (!$szMatch.Success) {
        Write-Error "Could not find BASEVERSION_SZ in file."
        exit 1
    }

    $baseVersionSz = $szMatch.Groups[1].Value

    $binPattern = '#define\s+BASEVERSION_BIN\s+([0-9]+,[0-9]+,[0-9]+,[0-9]+)'
    $binMatch = [regex]::Match($content, $binPattern)

    if (!$binMatch.Success) {
        Write-Error "Could not find BASEVERSION_BIN in file."
        exit 1
    }

    $baseVersionBin = $binMatch.Groups[1].Value

    $inputBin = $RequestedVersion -replace '\.', ','

    Write-Host "Checking RC file versions..."
    Write-Host " BASEVERSION_SZ : $baseVersionSz"
    Write-Host " BASEVERSION_BIN: $baseVersionBin"
    Write-Host " Input version  : $RequestedVersion ($inputBin)"

    $ok = $true;

    if ($baseVersionSz -ne $RequestedVersion) {
        Write-Error "BASEVERSION_SZ does not match input"
        $ok = $false;
    }

    if ($baseVersionBin -ne $inputBin) {
        Write-Error "BASEVERSION_BIN does not match input"
        $ok = $false;
    }

    if ($ok -ne $true) {
        Write-Error "Version mismatch!"
        exit 1
    }
}

function Check-GitRevision {
    $sourceVersion = "Unofficial"

    if (Get-Command git -ErrorAction SilentlyContinue) {
        # Get short commit hash
        $sourceVersion = git rev-parse --short HEAD

        Write-Host "Git found, tree is at revision: " $sourceVersion
    } else {
        Write-Error "Git not found."
    }

    $gitRevisionFile = $PSScriptRoot + "\..\" + "src\gitversion.h"

    if (!(Test-Path $gitRevisionFile)) {
        Write-Error "File not found: $gitRevisionFile"
        exit 1
    }

    $gitRevisionContent = Get-Content $gitRevisionFile -Raw

    $gitRevisionPattern = '#define\s+GITVERSION\s+"([^"]+)"'

    $match = [regex]::Match($gitRevisionContent, $gitRevisionPattern)

    if ($match.Success) {
        $gitVersion = $match.Groups[1].Value
        Write-Host "Source revision: $gitVersion"
    } else {
        Write-Error "GITVERSION not found in file."
        exit 1
    }

    if ($gitVersion -ne $sourceVersion) {
        Write-Error "Git revision $gitVersion does not match the source version $sourceVersion"
        exit 1
    }
}

function Create-ReleaseArchive {
    param(
        [Parameter(Mandatory=$true)]
        [string] $RequestedVersion
    )

    $buildRoot = $PSScriptRoot + "\..\" + "msvc\build\"
    $releaseRoot = $buildRoot + "archive"
    $32bitRoot = $releaseRoot + "\x86"
    $64bitRoot = $releaseRoot + "\x64"
    $32bitBinary = $buildRoot + "bin\srvany-ng\Release Unicode_Win32\srvany-ng_ru32.exe"
    $64bitBinary = $buildRoot + "bin\srvany-ng\Release Unicode_x64\srvany-ng_ru64.exe"
    $destinationArchive = $releaseRoot + "\srvany-ng-" + $RequestedVersion + ".zip"

    Remove-Item -Path $releaseRoot -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $releaseRoot
    New-Item -ItemType Directory -Path $32bitRoot
    New-Item -ItemType Directory -Path $64bitRoot

    if (!(Test-Path $32bitBinary)) {
        Write-Error "File not found: $32bitBinary"
        exit 1
    }

    if (!(Test-Path $64bitBinary)) {
        Write-Error "File not found: $64bitBinary"
        exit 1
    }

    Copy-Item -Path $32bitBinary -Destination ($32bitRoot + "\srvany-ng.exe")
    Copy-Item -Path $64bitBinary -Destination ($64bitRoot + "\srvany-ng.exe")

    Compress-Archive -Path $32bitRoot, $64bitRoot -DestinationPath $destinationArchive -Force

    Write-Host "Created release archive: $destinationArchive"
}

Check-RcFileVersion -RequestedVersion $Version
Check-GitRevision
Create-ReleaseArchive -RequestedVersion $Version
