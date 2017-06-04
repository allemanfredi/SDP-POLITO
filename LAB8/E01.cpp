/* The program has to run N threads (one for each argument).

Each thread recursively visits one of the directories, and,

for each directory entry, it prints-out its thread identifier

and the directory entry name.

The main thread awaits for the termination of all threads.

When all threads have visited their directory also the program

ends.



Version A

In the previous version (Version A), as all output lines are

generated independently by each single thread, printing messages

from different threads are interleaved on standard output.

To avoid interleaved output it is usually possibile to proceed

in several ways.

Then, extend version A as follows.*/



#include "stdafx.h"

#define UNICODE
#define _UNICODE


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>


#define CREATE_NOT_SUSPEND		0

#define TYPE_FILE				1
#define TYPE_DIR				2
#define TYPE_DOT				3

#define BUFFER					500


typedef struct _THREAD_DATA {

	DWORD			level;
	LPTSTR			path;

}THREAD_DATA;


static DWORD FileType(LPWIN32_FIND_DATA pFileData);
DWORD WINAPI TraverseDirectoryRecursive(PVOID param);

int _tmain(int argc, LPTSTR argv[]) {

	
	DWORD			numArgument;
	DWORD			numThread;
	PHANDLE			tHandle;
	PDWORD			tId;
	THREAD_DATA		*d;


	if (argc < 2)
	{
		fprintf(stderr, "Error: Please insert at least one absolute or relative path\n");
		return -1;
		system("pause");
	}

	//getting number of argument = number of thread
	numThread = numArgument = argc - 1;

	//thread allocation
	tHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	tId = (PDWORD)malloc(numThread * sizeof(DWORD));

	//stucture allocation for thread
	d = (THREAD_DATA *)malloc(numThread * sizeof(THREAD_DATA));

	//thread creation
	for (DWORD iThread = 0; iThread < numThread; iThread++)
	{
		d[iThread].path = argv[iThread + 1]; //+ 1 because the first argument is the program name
		d[iThread].level = 0;

		tHandle[iThread] = CreateThread(NULL, 0, TraverseDirectoryRecursive, &d[iThread], CREATE_NOT_SUSPEND, &tId[iThread]);
	}


	// wait all threads
	WaitForMultipleObjects(numThread, tHandle, TRUE, INFINITE);


	system("pause");

}


DWORD WINAPI TraverseDirectoryRecursive(PVOID param)
{
	HANDLE				SearchHandle;
	WIN32_FIND_DATA		FindData;
	DWORD				FType, i;
	THREAD_DATA			*data;
	DWORD				id;

	id = GetCurrentThreadId();

	data = (THREAD_DATA *)param;
	
	SetCurrentDirectory(data->path);
	SearchHandle = FindFirstFile(_T("*"), &FindData);

	do {
		FType = FileType(&FindData);
		if (FType == TYPE_FILE) {
			/* Printing */
			for (i = 0; i<data->level; i++)
				_tprintf(_T("  "));
			_tprintf(_T("Thread ID %d ---> level=%d FILE: %s\n"), id , data->level,FindData.cFileName);
		}
		
			if (FType == TYPE_DIR) {
				/* Printing */
				for (i = 0; i<data->level; i++)
					_tprintf(_T("  "));
				_tprintf(_T("Thread ID %d ---> level=%d DIR : %s\n"), id , data->level,FindData.cFileName);

				THREAD_DATA d;
				d.level = data->level + 1;
				d.path = FindData.cFileName;

				TraverseDirectoryRecursive(&d);
				SetCurrentDirectory(_T(".."));
			}
	} while (FindNextFile(SearchHandle, &FindData));
	FindClose(SearchHandle);
	return 1;
}



static DWORD FileType(LPWIN32_FIND_DATA pFileData)
{
	BOOL IsDir;
	DWORD FType;
	FType = TYPE_FILE;
	IsDir = (pFileData->dwFileAttributes &
		FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (IsDir)
		if (lstrcmp(pFileData->cFileName, _T(".")) == 0
			|| lstrcmp(pFileData->cFileName, _T("..")) == 0)
			FType = TYPE_DOT;
		else FType = TYPE_DIR;
		return FType;

}


