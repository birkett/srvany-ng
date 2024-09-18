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
#include <time.h>
#include <tchar.h> //For _tmain().

#define MAX_DATA_LENGTH 8192

//Replace the calls to printf and ctime for MBCS and UNICODE builds.
#ifdef UNICODE
    #include <wchar.h> //For wprintf().
    #define printf_w wprintf
    #define wctime _wctime_s
#else
    #define printf_w printf
    #define wctime ctime_s
#endif


/*
 * Wrapper to send output to the console and output file with one call.
 *  If fpFile is NULL, only output to the console.
 */
void writeOutput(FILE* fpFile, TCHAR* szString)
{
    printf_w(TEXT("%s\n"), szString);
    if (fpFile != NULL)
    {
        fwrite(szString, sizeof(TCHAR), lstrlen(szString), fpFile);
        fwrite("\n", sizeof(char), 1, fpFile);
    }
}//end writeOutput()


/*
 * Simple target application to aid testing. 
 *  Prints current directory, environment and parameters to check if srvany-ng
 *  is starting apps with the correct values.
 */
int _tmain(int argc, TCHAR *argv[])
{
    TCHAR* currentDirectory   = (TCHAR*)calloc(MAX_DATA_LENGTH, sizeof(TCHAR));
    TCHAR* currentEnvironment = (TCHAR*)calloc(MAX_DATA_LENGTH, sizeof(TCHAR));
    TCHAR* timeBuffer         = (TCHAR*)calloc(      26       , sizeof(TCHAR));
    FILE*  outputFile         = NULL;
    time_t timeData;

    if (currentDirectory == NULL || currentEnvironment == NULL || timeBuffer == NULL)
    {
        writeOutput(NULL, TEXT("calloc() failed"));
        return ERROR_OUTOFMEMORY;
    }

    if (fopen_s(&outputFile, "output.txt", "w") != ERROR_SUCCESS)
    {
        writeOutput(NULL, TEXT("fopen() failed"));
        return ERROR_OPEN_FAILED;
    }

    time(&timeData);
    
    if (wctime(timeBuffer, 26, &timeData) != ERROR_SUCCESS)
    {
        writeOutput(NULL, TEXT("ctime() failed"));
        return ERROR_INVALID_PARAMETER;
    }

    writeOutput(outputFile, TEXT("*** Starting test"));
    writeOutput(outputFile, timeBuffer);

    GetCurrentDirectory(MAX_DATA_LENGTH, currentDirectory);
    currentEnvironment = GetEnvironmentStrings();

    writeOutput(outputFile, TEXT("\n\n*** Current directory:"));
    writeOutput(outputFile, currentDirectory);

    TCHAR* envarPointer = currentEnvironment;

    writeOutput(outputFile, TEXT("\n\n*** Current environment:"));

    while (*envarPointer)
    {
        writeOutput(outputFile, envarPointer);
        envarPointer += lstrlen(envarPointer) + 1;
    }

    writeOutput(outputFile, TEXT("\n\n*** Argv[] contents:"));

    for (int i = 0; i < argc; i++)
    {
        writeOutput(outputFile, argv[i]);
    }

    writeOutput(outputFile, TEXT("\n\n***End test"));
    fclose(outputFile);
    return ERROR_SUCCESS;
}//end main()
