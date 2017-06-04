
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>



#define		BUFFER					500
#define		DATA_BUFFER				30
#define		CREATE_NOT_SUSPEND		0

typedef struct _THREAD_DATA {
	
	TCHAR			nameOperationFile[BUFFER];
	TCHAR			nameAccountFile[BUFFER];


}THREAD_DATA;


typedef struct _FILE_DATA {

	DWORD			id;
	DWORD			accountNumber;
	TCHAR			name[DATA_BUFFER];
	TCHAR			surname[DATA_BUFFER];
	INT 			number;		// for "account file" this number is used to get the account balance amount while for the operation
								// file is used to store the deposit or withdrawal

}FILE_DATA;


//proptoptype
DWORD WINAPI work(PVOID param);


INT _tmain(INT argc, LPTSTR argv[])
{
	PHANDLE			threadHandle;
	PDWORD			threadId;
	DWORD			numThread;
	HANDLE			hAccountFile;

	THREAD_DATA	    *arrayOfThreadData;
	FILE_DATA		dd;
	DWORD			nIn;

	 
	

	//number of thread calculating
	numThread = argc - 2;   // -2 because one is for the program name and one for the "ACCOUNT" file
	

	//thread allocation
	threadHandle = (PHANDLE) malloc(numThread * sizeof(HANDLE));
	threadId	 = (PDWORD)  malloc(numThread * sizeof(DWORD));

	
	//data structure to pass to the thread
	arrayOfThreadData = (THREAD_DATA *)malloc(numThread * sizeof(THREAD_DATA));



	
	for (int iThread = 0; iThread < numThread; iThread++) {

		//copy file name
		_tcscpy(arrayOfThreadData[iThread].nameOperationFile, argv[iThread + 2]);  // pass to each tread the name of operation file

		//copyt account fle name
		_tcscpy(arrayOfThreadData[iThread].nameAccountFile, argv[1]);  // pass to each tread the name of operation file


		//thread creation
		threadHandle[iThread] = CreateThread(NULL, 0, work , &arrayOfThreadData[iThread], CREATE_NOT_SUSPEND, &threadId[iThread]);

	}
	
	
	//wait all thread
	WaitForMultipleObjects(numThread, threadHandle, TRUE, INFINITE);

	//close the "account file" once that all thread are done
	
	// close the heandle refers to the thread
	for (int i = 0; i < numThread; i++)
		CloseHandle(threadHandle[i]);
	

	
	
	hAccountFile = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hAccountFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return -2;
	}


	while (ReadFile(hAccountFile, &dd, sizeof(FILE_DATA), &nIn, NULL) > 0 && nIn > 0) {

		_tprintf(_T("name : %s\n"), dd.name);
		_tprintf(_T("surname : %s\n"), dd.surname);
		_tprintf(_T("Account Number : %d\n"), dd.accountNumber);
		_tprintf(_T("id : %d\n"), dd.id);
		_tprintf(_T("Amount of money : %d\n\n"), dd.number);
		
	}

	
	system("PAUSE");
	CloseHandle(hAccountFile);

	return 0;
}



DWORD WINAPI work(PVOID param) {

	THREAD_DATA			*data;
	HANDLE				hOperationFile, hAccountFile;
	DWORD				nIn, nOut;
	FILE_DATA			fileData, fileDataApp;
	OVERLAPPED			overlapped = { 0, 0, 0, 0, NULL };
	LARGE_INTEGER		filePos;
	LARGE_INTEGER		fileReserved;

	

	data = (THREAD_DATA *)param;


	//opening operation fle
	hOperationFile = CreateFile(data->nameOperationFile, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOperationFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input operation file: Error: %x\n", GetLastError());
		return -2;
	}


	//opening the account file
	hAccountFile = CreateFile(data->nameAccountFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hAccountFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open account file. Error: %x\n", GetLastError());
		return -2;
	}


	
	while (ReadFile(hOperationFile, &fileData, sizeof(FILE_DATA), &nIn, NULL) > 0 && nIn > 0){

		//loking generation;
		filePos.QuadPart		=  (fileData.id - 1) * sizeof(FILE_DATA);  //set the position on the file
		fileReserved.QuadPart	=  sizeof(FILE_DATA);				// block only one row

		//overlapped strcutre generation
		overlapped.Offset     = filePos.LowPart; 
		overlapped.OffsetHigh = filePos.HighPart; 
		overlapped.hEvent     = (HANDLE)0;



		//lock the row in order to change the amount of avaiable money
		if (LockFileEx(hAccountFile, LOCKFILE_EXCLUSIVE_LOCK, 0, fileReserved.LowPart, fileReserved.HighPart, &overlapped) == FALSE) {
			fprintf(stderr, "Error %x", GetLastError());
			exit(EXIT_FAILURE);
		}

		
		//R E C O R D   U P D A T I N G
		//here i have to set the position in "account" file in order to change the amount of money
		if (ReadFile(hAccountFile, &fileDataApp, sizeof(FILE_DATA), &nIn, &overlapped) == FALSE) {
			fprintf(stderr, "Error %x", GetLastError());
			exit(EXIT_FAILURE);
		}
		
		fileDataApp.number+=fileData.number; /* Update the record by adding or subtracting the read value */
		
		
		//write the updated ecord on the file in the same position in which has been read
		if (WriteFile(hAccountFile, &fileDataApp, sizeof(FILE_DATA), &nOut, &overlapped) == FALSE) {
			fprintf(stderr, "Error %x", GetLastError());
			exit(EXIT_FAILURE);
		}
	
		//unclock the row so that the oder thrread cann access to it
		UnlockFileEx(hAccountFile, 0, fileReserved.LowPart , fileReserved.HighPart, &overlapped);

	}

	
	//file closing
	CloseHandle(hOperationFile);
	CloseHandle(hAccountFile);


	return 0;
}
