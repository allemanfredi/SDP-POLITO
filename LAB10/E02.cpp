
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>



#define		BUFFER					500
#define		DATA_BUFFER				30
#define		CREATE_NOT_SUSPEND		0

typedef struct _THREAD_DATA {

	TCHAR			nameOperationFile[BUFFER];
	TCHAR			nameAccountFile[BUFFER];
	PCRITICAL_SECTION	cs;


}THREAD_DATA;


typedef struct _FILE_DATA {

	DWORD			id;
	DWORD			accountNumber;
	TCHAR			name[DATA_BUFFER];
	TCHAR			surname[DATA_BUFFER];
	INT				number;		// for "account file" this number is used to get the account balance amount while for the operation
								// file is used to store the deposit or withdrawal

}FILE_DATA;


//proptoptype
DWORD WINAPI work(PVOID param);


INT _tmain(INT argc, LPTSTR argv[])
{
	PHANDLE				threadHandle;
	PDWORD				threadId;
	DWORD				numThread;
	HANDLE				hAccountFile;

	THREAD_DATA			*arrayOfThreadData;
	FILE_DATA			dd;
	DWORD				nIn;
	CRITICAL_SECTION	cs;




	//number of thread calculating
	numThread = argc - 2;   // -2 because one is for the program name and one for the "ACCOUNT" file


	//thread allocation
	threadHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	threadId = (PDWORD)malloc(numThread * sizeof(DWORD));


	//data structure to pass to the thread
	arrayOfThreadData = (THREAD_DATA *)malloc(numThread * sizeof(THREAD_DATA));

	
	//critical section declaration
	InitializeCriticalSection(&cs);

	

	for (int iThread = 0; iThread < numThread; iThread++) {

		//copy file name
		_tcscpy(arrayOfThreadData[iThread].nameOperationFile, argv[iThread + 2]);  // pass to each tread the name of operation file


		//passing the hande to account file just opened
		arrayOfThreadData[iThread].hAccountFile = hAccountFile;

		//critical section assignment
		arrayOfThreadData[iThread].cs = &cs;

		//thread creation
		threadHandle[iThread] = CreateThread(NULL, 0, work, &arrayOfThreadData[iThread], CREATE_NOT_SUSPEND, &threadId[iThread]);

	}


	//wait all thread
	WaitForMultipleObjects(numThread, threadHandle, TRUE, INFINITE);


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

		fprintf(stdout, "name : %s\n", dd.name);
		fprintf(stdout, "surname : %s\n", dd.surname);
		fprintf(stdout, "Account Number : %d\n", dd.accountNumber);
		fprintf(stdout, "id : %d\n", dd.id);
		fprintf(stdout, "Amount of money : %d\n\n", dd.number);

	}


	system("PAUSE");
	CloseHandle(hAccountFile);

	return 0;
}



DWORD WINAPI work(PVOID param) {

	THREAD_DATA			*data;
	HANDLE				hOperationFile;
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
		fprintf(stderr, "Cannot open input file thread. Error: %x\n", GetLastError());
		return -2;
	}


	//opening the account file
	hAccountFile = CreateFile(argv[1], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hAccountFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return -2;
	}
	

	while (ReadFile(hOperationFile, &fileData, sizeof(FILE_DATA), &nIn, NULL) > 0 && nIn > 0) {

		//loking generation;
		filePos.QuadPart = (fileData.id - 1) * sizeof(FILE_DATA);  //set the position on the file
		fileReserved.QuadPart = sizeof(FILE_DATA);				// block only one row

																//overlapped strcutre generation
		overlapped.Offset = filePos.LowPart;
		overlapped.OffsetHigh = filePos.HighPart;
		overlapped.hEvent = (HANDLE)0;


		EnterCriticalSection(data->cs);

		//R E C O R D   U P D A T I N G
		//here i have to set the position in "account" file in order to change the amount of money
		ReadFile(hAccountFile, &fileDataApp, sizeof(FILE_DATA), &nIn, &overlapped);

		fileDataApp.number += fileData.number; /* Update the record by adding or subtracting the read value */

											   //write the updated ecord on the file in the same position in which has been read
		WriteFile(hAccountFile, &fileDataApp, sizeof(FILE_DATA), &nOut, &overlapped);

		LeaveCriticalSection(data->cs);



	}

	
	//file closing
	CloseHandle(hOperationFile);

	CloseHandle(hAccountFile);

	return 0;
}
