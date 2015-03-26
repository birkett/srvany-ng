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
#include <stdio.h>
#include <tchar.h> //For _tmain().

#define MAX_DATA_LENGTH 8192

//Replace the calls to printf for MBCS and UNICODE builds.
#ifdef UNICODE
    #include <wchar.h> //For wprintf().
    #define printf_w wprintf
#else
    #define printf_w printf
#endif

/*
 * Simple target application to aid testing. 
 *  Prints current directory, environment and parameters to check if srvany-ng
 *  is starting apps with the correct values.
 */
int _tmain(int argc, TCHAR *argv[])
{
    TCHAR* currentDirectory   = (TCHAR*)calloc(MAX_DATA_LENGTH, sizeof(TCHAR));
    TCHAR* currentEnvironment = (TCHAR*)calloc(MAX_DATA_LENGTH, sizeof(TCHAR));

    if (currentDirectory == NULL || currentEnvironment == NULL)
    {
        printf_w(TEXT("calloc() failed\n"));
        return ERROR_OUTOFMEMORY;
    }

    FILE* outputFile = fopen("output.txt", "w");

    if (outputFile == NULL)
    {
        printf_w(TEXT("fopen() failed\n"));
        return ERROR_OPEN_FAILED;
    }

    GetCurrentDirectory(MAX_DATA_LENGTH, currentDirectory);
    currentEnvironment = GetEnvironmentStrings();

    printf_w(TEXT("%s\n"), currentDirectory);
    fwrite(currentDirectory, sizeof(TCHAR), lstrlen(currentDirectory), outputFile);
    fwrite("\n", sizeof(char), 1, outputFile);

    TCHAR* envarPointer = currentEnvironment;

    while (*envarPointer)
    {
        printf_w(TEXT("%s\n"), envarPointer);
        fwrite(envarPointer, sizeof(TCHAR), lstrlen(envarPointer), outputFile);
        fwrite("\n", sizeof(char), 1, outputFile);
        envarPointer += lstrlen(envarPointer) + 1;
    }

    for (int i = 0; i < argc; i++)
    {
        printf_w(TEXT("%s\n"), argv[i]);
        fwrite(argv[i], sizeof(TCHAR), lstrlen(argv[i]), outputFile);
        fwrite("\n", sizeof(char), 1, outputFile);
    }

    return ERROR_SUCCESS;
}//end main()
