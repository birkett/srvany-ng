git --version >nul 2>&1 && (
    echo Git found, source is at revision:
    for /f "delims=*" %%G in ('git rev-parse --short HEAD') do (@set SOURCEVERSION=%%G)
    git rev-parse --short HEAD
) || (
    echo Git not found.
    @set SOURCEVERSION=Unofficial
)

copy /y ..\tools\gitversion.template ..\src\gitversion.h

echo #define GITVERSION "%SOURCEVERSION%" >> ..\src\gitversion.h
exit 0
