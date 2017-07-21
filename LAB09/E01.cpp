#include "stdafx.h"

#define UNICODE
#define _UNICODE


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>

#define MAX_THREAD				10	
#define CREATE_NOT_SUSPEND		0

struct threadData {

	LPTSTR			filename;
	LPINT			vector;
	INT			nNumber;

};



DWORD WINAPI WorkTh(PVOID pThNum);
int compare(const void *a, const void *b);
LPINT merge(LPINT array0, LPINT array1, INT n1, INT n2);
PHANDLE GetNewArrayOfHandle(PHANDLE vet, HANDLE handle, DWORD dimVet);
threadData *GetNewArrayOfData(threadData *vet, INT handle, DWORD dimVet);


int _tmain(int argc, LPTSTR argv[])
{
	DWORD			numThread;
	PHANDLE			threadHandle;
	PDWORD			threadId;
	threadData		*d;
	DWORD			sizeFinalVet = 0;
	LPINT			finalVet = NULL;
	DWORD			result = 0;
	DWORD			terminate = 0;
	DWORD			lastTerminate = 0;



	numThread = argc - 2; //number of arguments minus 2 seeing thata the last one parameters is the output name and the first one is the program name
	if (numThread > MAX_THREAD)
	{
		_tprintf(_T("Num max of thread has been excedeed \n"));
		return -1;
	}


	threadHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	threadId = (PDWORD)malloc(numThread * sizeof(DWORD));
	d = (threadData *)malloc(numThread * sizeof(threadData));

	//HANDLE DECLARATION
	for (int iThread = 0; iThread < numThread; iThread++)
	{
		d[iThread].filename = argv[iThread + 1]; //+ 1 because the first argument is the program name

		threadHandle[iThread] = CreateThread(NULL, 0, WorkTh, &d[iThread], CREATE_NOT_SUSPEND, &threadId[iThread]);

	}


	// wait all threads
				

	

	while ( terminate < numThread )
	{
		result = WaitForMultipleObjects(numThread - terminate, threadHandle, FALSE, INFINITE ); // I use "false" in order to not wait all thread but only one
		
		result -= WAIT_OBJECT_0;

		// generate the merged vector
		finalVet = merge(d[result].vector, finalVet, d[result].nNumber, sizeFinalVet);
		sizeFinalVet += d[result].nNumber;
		
		//generate new vector seeing that waitofrmultipleobject function has deleted the handle which refers to the terminated thread
		threadHandle = GetNewArrayOfHandle(threadHandle, threadHandle[result], numThread - terminate);
		d = GetNewArrayOfData(d, result, numThread - terminate);
		

		terminate++;

		
	}
	fprintf(stdout, " \n\n\n");
	for ( int i = 0; i < sizeFinalVet; i++)
		fprintf(stdout, " %d\n", finalVet[i]);

	system("pause");

}


LPINT merge(LPINT array0, LPINT array1, INT n1, INT n2) {
	INT smallestnum, i, smallestNumIndex;
	INT numMerge = n1 + n2;
	INT* mergeNums = (INT*)malloc(((numMerge) * sizeof(INT)));
	INT* mergeIndexes = (INT*)malloc(((2) * sizeof(INT)));
	INT mergeMaxIndexes[2] = { n1, n2 };

	mergeIndexes[0] = 0;
	mergeIndexes[1] = 0;


	for (i = 0; i < numMerge; i++) {
		smallestnum = INT_MAX;
		//array 1
		if (mergeMaxIndexes[0] > mergeIndexes[0]) {
			smallestnum = array0[mergeIndexes[0]];
			smallestNumIndex = 0;
		}

		//array 2
		if (mergeMaxIndexes[1] > mergeIndexes[1]) {
			if (smallestnum > array1[mergeIndexes[1]]) {
				smallestnum = array1[mergeIndexes[1]];
				smallestNumIndex = 1;
			}
		}

		mergeNums[i] = smallestnum;
		mergeIndexes[smallestNumIndex]++;

	}


	free(mergeIndexes);

	return mergeNums;
}






PHANDLE GetNewArrayOfHandle(PHANDLE vet, HANDLE handle , DWORD dimVet)
{

	if (dimVet == 0)
		return NULL;

	PHANDLE finalHandleVet = (PHANDLE)malloc((dimVet - 1) * sizeof(HANDLE));

	INT j = 0;
	for (INT i = 0; i < dimVet; i++) {

		if (vet[i] != handle) {
			finalHandleVet[j] = vet[i];
			j++;
		}
	}
		

	return finalHandleVet;
	

}


threadData *GetNewArrayOfData(threadData *vet, INT handle, DWORD dimVet)
{

	if (dimVet == 0)
		return NULL;

	threadData *finalDataVet = (threadData *)malloc((dimVet - 1) * sizeof(threadData));

	
	INT j = 0;
	for (INT i = 0; i < dimVet; i++) {

		if (i != handle) {
			finalDataVet[j] = vet[i];
			j++;
		}
	}


	return finalDataVet;

}



DWORD WINAPI WorkTh(PVOID pThNum) {

	threadData			*data;
	HANDLE				hIn;
	DWORD				nNumber;
	DWORD				nIn;
	DWORD				numRead;

	data = (threadData *)pThNum;


	//file opening
	hIn = CreateFile(data->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE)
		ExitThread(-1);

	//read the number of numbers i have to read
	nIn = ReadFile(hIn, &nNumber, sizeof(INT32), &nIn, NULL);
		data->nNumber = nNumber;

	//vector allocation
	data->vector = (LPINT)malloc(nNumber * sizeof(INT32));

	//get number
	for (UINT16 i = 0; i < nNumber; i++)
	{
		nIn = ReadFile(hIn, &numRead, sizeof(INT32), &nIn, NULL);
		data->vector[i] = numRead;
	}

	qsort(data->vector, nNumber, sizeof(INT32), compare);

	CloseHandle(hIn);
	ExitThread(0);
}


int compare(const void *a, const void *b)
{
	/* Compare all of both strings: */
	return ((INT32)a - (INT32)b);
}





