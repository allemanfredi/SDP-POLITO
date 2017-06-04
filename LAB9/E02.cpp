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

	DWORD				level;
	TCHAR				path[BUFFER];
	DWORD				id;

	PHANDLE				semaphoreForComparing;
	PHANDLE				semaphoreForReading;
	
	TCHAR				nameForComparing[BUFFER];
	DWORD				indexToAccessFileNameToCompare;




}THREAD_DATA;


typedef struct _THREAD_DATA_COMPARING {

	PHANDLE				semaphoreForComparing;
	PHANDLE				semaphoreForReading;


	THREAD_DATA			*vetData;

	DWORD				numThread;

	PHANDLE				vetReadingThreadHandle;

}THREAD_DATA_COMPARING;



//global varable
TCHAR			**vetFileNameToCompare;  // numThread * 500 
DWORD			terminate;				// used to keep trace of the terminated thread



//prototype
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
DWORD WINAPI TraverseDirectoryRecursive(PVOID param);
DWORD WINAPI Comparing(PVOID param);



int _tmain(int argc, LPTSTR argv[]) {


	DWORD					numArgument;
	DWORD					numThread;

	PHANDLE					readingThreadHandle;
	PDWORD					readingThreadId;

	THREAD_DATA				*d;
	THREAD_DATA_COMPARING	dComparing;

	HANDLE					comparingThreadHanlde;
	DWORD					comparingThreadId;

	HANDLE					semaphoreforComparing;
	HANDLE					semaphoreforReading;

	DWORD					result;



	if (argc < 2)
	{
		fprintf(stderr, "Error: Please insert at least one absolute or relative path\n");
		return -1;
		system("pause");
	}


	//getting number of argument = number of thread
	numThread = numArgument = argc - 1;



	//thread allocation
	readingThreadHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	readingThreadId     = (PDWORD)malloc(numThread * sizeof(DWORD));


	//handle allocaton for the structure to pass to the comparing thread
	dComparing.vetReadingThreadHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));


	// global vector used to store the file  or direotry name to be compared with the others 
	vetFileNameToCompare = (PTCHAR *)malloc(numThread * sizeof(PTCHAR));
	for (INT i = 0; i < numThread; i++)
		vetFileNameToCompare[i] = (PTCHAR)malloc(BUFFER * sizeof(TCHAR));
	

	//array for comparing intialized to 0
	for ( int i = 0; i < numThread; i++)
		memset(vetFileNameToCompare[i], 0, BUFFER);
  


	//stucture allocation for thread
	d = (THREAD_DATA *)malloc(numThread * sizeof(THREAD_DATA));


	
	//array of strucutre to pass to the comapring thread;
	dComparing.vetData = (THREAD_DATA *)malloc(numThread * sizeof(THREAD_DATA));


	//semaphore creation
	semaphoreforReading   = CreateSemaphore(NULL, 0, numThread, NULL);
	semaphoreforComparing = CreateSemaphore(NULL, 0, numThread, NULL);

	
	//thread reading creation = number of pathS inserted by the user
	for (DWORD iThread = 0; iThread < numThread; iThread++)
	{
		_tcscpy(d[iThread].path , argv[iThread + 1]); //+ 1 because the first argument is the program name
		d[iThread].level = 0;
		d[iThread].id = iThread;


		//semaphore assigning ( by passing the address in ordet to make visible the same semaphore at each thread)
		d[iThread].semaphoreForComparing = &semaphoreforComparing;
		d[iThread].semaphoreForReading   = &semaphoreforReading;


		//assing the index to acces to global array which store the filename to compare
		d[iThread].indexToAccessFileNameToCompare = iThread;


		//fill array of data strucutre to pass to the comparing array
		dComparing.vetData[iThread] = d[iThread];


		//thread creation
		readingThreadHandle[iThread] = CreateThread(NULL, 0, TraverseDirectoryRecursive, &d[iThread], CREATE_NOT_SUSPEND, &readingThreadId[iThread]);
	}


	//memory allocation for the array of id for passing to comparing thread
	dComparing.vetReadingThreadHandle = readingThreadHandle;


	//filling comparign thread data stucutre
	dComparing.numThread = numThread;


	// passing the sempahore to the comparing thread
	dComparing.semaphoreForComparing = &semaphoreforComparing;
	dComparing.semaphoreForReading = &semaphoreforReading;


	//comparing thread creation
	comparingThreadHanlde = CreateThread(NULL, 0, Comparing, &dComparing, CREATE_NOT_SUSPEND, &comparingThreadId);


	
	//wait for comparing thread
	WaitForSingleObject(comparingThreadHanlde, INFINITE);

	//wait for READING threads
	WaitForMultipleObjects(numThread, readingThreadHandle, TRUE, INFINITE);

	

	//get the exit code in order to derminen the result...in negative case i have to kill all remained process
	GetExitCodeThread(comparingThreadHanlde, &result);
	
	
	if (result == 0)
		_tprintf(_T("Directory are equals\n"));

	else if (result == -1)
		_tprintf(_T("Directory are NOT equals\n"));


	system("pause");
	return 0;

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



DWORD WINAPI Comparing(PVOID param)
{
	THREAD_DATA_COMPARING			*data;
	DWORD							indexVetFileToCompare;
	DWORD							resultOfCompare;   // 0 positive   -1 negative


	//get the structure
	data = (THREAD_DATA_COMPARING *)param;


	//variable initialization
	indexVetFileToCompare = data->numThread;
	resultOfCompare = 0;


	while (terminate < data->numThread  ) 
	{

		//wait the filename from the reading thread
		for (int i = 0; i < data->numThread - terminate ; i++)
			WaitForSingleObject(*data->semaphoreForComparing, INFINITE);
		
	
		

		//compare all filename 
		for (int i = 0; i < indexVetFileToCompare - 1; i++)
			if (_tcscmp(vetFileNameToCompare[i], vetFileNameToCompare[i+1]) != 0)
			{
				// kill all remained process
				for (int j = 0; j < data->numThread; j++)
					TerminateThread(data->vetReadingThreadHandle[j], -1);
				
				// exit seei that the directory are not the same
				ExitThread(-1);

			}

		
		//restart the remained thread
		for (int i = 0; i < data->numThread - terminate; i++)
			ReleaseSemaphore(*data->semaphoreForReading, 1, NULL);

	}

	
	ExitThread(0);
}




DWORD WINAPI TraverseDirectoryRecursive(PVOID param)
{
	HANDLE				SearchHandle;
	WIN32_FIND_DATA		FindData;
	DWORD				FType, i;
	THREAD_DATA			*data;
	DWORD				id;
	TCHAR				originalPath[BUFFER] = { 0 };
	TCHAR				allNames[BUFFER] = { 0 };

	//get thread id
	id = GetCurrentThreadId();

	data = (THREAD_DATA *)param;

	//used for recursione
	_tcscpy(originalPath, data->path);


	//absolute path genearatio
	_tcscpy(allNames, data->path);
	_tcscat(allNames, _T("\\*"));

	SearchHandle = FindFirstFile(allNames, &FindData);


	do {
		FType = FileType(&FindData);
	

		if (FType == TYPE_FILE) {

			//set the name to be compared
			_tcscpy(vetFileNameToCompare[data->indexToAccessFileNameToCompare], FindData.cFileName);

			//increase the semaphore in order to say to comparin thread that it must to start the comparing
			ReleaseSemaphore(*data->semaphoreForComparing, 1, NULL);

			//wait the response from the comparing thread
			WaitForSingleObject(*data->semaphoreForReading, INFINITE);



		}
		

		if (FType == TYPE_DIR) {


			//set the name to be compared
			_tcscpy(vetFileNameToCompare[data->indexToAccessFileNameToCompare], FindData.cFileName);

			//increase the semaphore in order to say to comparin thread that it must to start the comparing
			ReleaseSemaphore(*data->semaphoreForComparing, 1, NULL);

			//wait the response from the comparing thread
			WaitForSingleObject(*data->semaphoreForReading, INFINITE);


			//generate new absoulte path (only for rireotry);
			_tcscat(data->path, _T("\\"));
			_tcscat(data->path, FindData.cFileName);
			

			//generate the new structure to pass it to the recursive function ( level + 1)
			THREAD_DATA d;
			d.level = data->level + 1;
			_tcscpy( d.path , data->path);
			d.semaphoreForComparing = data->semaphoreForComparing;
			d.semaphoreForReading = data->semaphoreForReading;
			d.indexToAccessFileNameToCompare = data->indexToAccessFileNameToCompare;


			TraverseDirectoryRecursive(&d);

			_tcscpy(data->path, originalPath);

		}
		
	
	} while (FindNextFile(SearchHandle, &FindData));


	FindClose(SearchHandle);

	
	
	//if level == 0 it means that thread will exit so i have to decrement the number of thread in exectuion
	if (data->level == 0)
	{
		terminate++;
		ReleaseSemaphore(*data->semaphoreForComparing, 1, NULL);
	
	}
	

	return 0;
}






