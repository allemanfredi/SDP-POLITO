
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE


#define		BUFFER					500
#define		CREATE_NOT_SUSPEND		0


typedef struct _RECORD {

	DWORD N;
	TCHAR sequenceOfCharacter[512];

} RECORD;


typedef struct _LIST {
	RECORD	    *buffer;  //list of all record
	INT			 size;
	INT			 read;
	INT			 write;

}LIST;

typedef struct _THREAD_A_DATA {

	DWORD				numFile;
	LIST				*allQueueA;			//thread A has to have access to all queue of kind A;
	LPTSTR				*listOfFileA;

	PDWORD				hashOpenedFileA;   // array to keep trace of opened array in order to do not open tha same file again
	
	//THREAD A ---- THREAD B
	PHANDLE				fillAB;
	PHANDLE				emptyAB;

	PCRITICAL_SECTION	csAB;

	PCRITICAL_SECTION	csFileOpening;   //critical section used to file opening


}THREAD_A_DATA;

typedef struct _THREAD_B_DATA {

	DWORD				numFile;
	LIST				*allQueueA;			//thread B has to have access to all queue of kind A;
	LIST				*allQueueB;			//thread B has to have access to all queue of kind B;

	//THREAD B ----- THREAD A
	PHANDLE				fillAB;
	PHANDLE				emptyAB;
	PCRITICAL_SECTION	csAB;

	//THREAD B ----- THRAD C
	PHANDLE				fillBC;
	PHANDLE				emptyBC;
	PCRITICAL_SECTION	csBC;

	

}THREAD_B_DATA;


typedef struct _THREAD_C_DATA {

	DWORD				numFile;
	LIST				*allQueueB;			//thread A has to have access to all queue of kind A;

	LPTSTR				*listOfFileB;

	PDWORD				hashOpenedFileC;

	//THREAD C ----- THREAD B
	PHANDLE				fillBC;
	PHANDLE				emptyBC;
	PCRITICAL_SECTION	csBC;


}THREAD_C_DATA;

BOOL	isDoneA = false;
BOOL	isDoneB = false;

DWORD	terminateA, terminateB;

//function protoype for queue
VOID init(LIST* ptr, INT size);
DWORD insert(LIST * ptr, RECORD value);
VOID visit(LIST * ptr);
RECORD *remove(LIST * ptr);
BOOL IsDone(PDWORD ptr, INT size);

//thread funciton protoype
DWORD WINAPI ThAWork(LPVOID param);
DWORD WINAPI ThBWork(LPVOID param);
DWORD WINAPI ThCWork(LPVOID param);


INT _tmain(INT argc, LPTSTR argv[])
{
	DWORD				numThread;
	TCHAR				*vetFileName;

	PHANDLE				thAHandle , thBHandle , thCHandle;
	PDWORD				thAId , thBId , thCId;

	LIST				*queueA, *queueB;

	THREAD_A_DATA		*dataThA;
	THREAD_B_DATA		*dataThB;
	THREAD_C_DATA		*dataThC;

	LPTSTR				nameOfInputFile[] = { L"data1.bin" , L"data2.bin" , L"data3.bin" , L"data4.bin", L"data5.bin" };
	LPTSTR				nameOfOutputFile[] = { L"output1.bin" , L"output2.bin" , L"output3.bin" , L"output4.bin", L"output5.bin" };

	HANDLE				fillAB,fillBC;
	HANDLE				emptyAB, emptyBC;
	CRITICAL_SECTION	csAB, csBC;

	CRITICAL_SECTION	csFileOpening;

	PDWORD				hashOpenedFileA, hashOpenedFileC;

	
	srand((unsigned)time(NULL));


	//number of thread = number of input file
	numThread = _tstoi(argv[1]);

	terminateA = numThread;
	terminateB = numThread;

	// T H R E A D   A   D A T A
	//handle allocation for thread A
	thAHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	thAId = (PDWORD)malloc(numThread * sizeof(DWORD));

	//queue A allocation
	queueA = (LIST *)malloc(numThread * sizeof(HANDLE));

	//structure for thread A allocation
	dataThA = (THREAD_A_DATA *)malloc(numThread * sizeof(THREAD_A_DATA));

	
	
	// T H R E A D   B   D A T A
	//handle allocation for thread B
	thBHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	thBId = (PDWORD)malloc(numThread * sizeof(DWORD));

	//queue B allocation
	queueB = (LIST *)malloc(numThread * sizeof(HANDLE));

	//structure for thread B allocation
	dataThB = (THREAD_B_DATA *)malloc(numThread * sizeof(THREAD_B_DATA));


	// T H R E A D   C   D A T A  (quque B just allocated )
	//handle allocation for thread C
	thCHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	thCId = (PDWORD)malloc(numThread * sizeof(DWORD));


	//structure for thread A allocation
	dataThC = (THREAD_C_DATA *)malloc(numThread * sizeof(THREAD_C_DATA));



	//semaphore and critical section initialization



	fillAB = CreateSemaphore(NULL, 0, numThread, NULL);
	emptyAB = CreateSemaphore(NULL, 1, numThread, NULL);

	fillBC = CreateSemaphore(NULL, 0, numThread, NULL);
	emptyBC = CreateSemaphore(NULL, 1, numThread, NULL);

	InitializeCriticalSection(&csAB);
	InitializeCriticalSection(&csBC);




	//critical section for file opening
	InitializeCriticalSection(&csFileOpening);


	//array to keep trace of file just opened 
	hashOpenedFileA = (PDWORD)malloc(numThread * sizeof(DWORD));
	for (int j = 0; j < numThread; j++)
		hashOpenedFileA[j] = 0;

	//array to keep trace of file just opened 
	hashOpenedFileC = (PDWORD)malloc(numThread * sizeof(DWORD));
	for (int j = 0; j < numThread; j++)
		hashOpenedFileC[j] = 0;


	//queue allocation
	queueA = (LIST *)malloc(numThread * sizeof(LIST));
	queueB = (LIST *)malloc(numThread * sizeof(LIST));

	for (int i = 0; i < numThread; i++) {
		init(&queueA[i], 10);
		init(&queueB[i], 10);
	}





	
	//thread A creation
	for (int i = 0; i < numThread; i++) {

		dataThA[i].numFile = numThread;

		dataThA[i].listOfFileA = (LPTSTR *)malloc(numThread * sizeof(LPTSTR));
		dataThA[i].listOfFileA = nameOfInputFile;

		//pass the pointer to the all queue A
		dataThA[i].allQueueA = queueA;

		//object for syncronization
		dataThA[i].emptyAB = &emptyAB;
		dataThA[i].fillAB = &fillAB;
		dataThA[i].csAB = &csAB;

		
		dataThA[i].hashOpenedFileA = hashOpenedFileA;

		dataThA[i].csFileOpening = &csFileOpening;


		thAHandle[i] = CreateThread(NULL, 0, ThAWork, &dataThA[i], CREATE_NOT_SUSPEND, &thAId[i]);
	}


	//thread B creation
	for (int i = 0; i < numThread; i++) {

		dataThB[i].numFile = numThread;

		//pass the pointer to the all queue A and B
		dataThB[i].allQueueA = queueA;
		dataThB[i].allQueueB = queueB;

		//object for syncronization
		dataThB[i].emptyAB = &emptyAB;
		dataThB[i].fillAB = &fillAB;
		dataThB[i].csAB = &csAB;

		dataThB[i].emptyBC = &emptyBC;
		dataThB[i].fillBC = &fillBC;
		dataThB[i].csBC = &csBC;


		thBHandle[i] = CreateThread(NULL, 0, ThBWork, &dataThB[i], CREATE_NOT_SUSPEND, &thBId[i]);
	}

	//thread C creation
	for (int i = 0; i < numThread; i++) {

		dataThC[i].numFile = numThread;

		dataThC[i].listOfFileB = (LPTSTR *)malloc(numThread * sizeof(LPTSTR));
		dataThC[i].listOfFileB = nameOfOutputFile;

		//pass the pointer to the all queue B
		dataThC[i].allQueueB = queueB;

		//object for syncronization
		dataThC[i].emptyBC = &emptyBC;
		dataThC[i].fillBC = &fillBC;
		dataThC[i].csBC = &csBC;

		//array to keep trace of file just opened 
		dataThC[i].hashOpenedFileC = hashOpenedFileC;



		thCHandle[i] = CreateThread(NULL, 0, ThCWork, &dataThC[i], CREATE_NOT_SUSPEND, &thCId[i]);
	}


	WaitForMultipleObjects(numThread, thAHandle, TRUE, INFINITE);

	
	isDoneA = true;

	if (ReleaseSemaphore(fillAB, numThread, NULL) == 0) //to wake up all thread blocked on waitforsingleobejct
		printf("Error: %x\n", GetLastError());
	

	WaitForMultipleObjects(numThread, thBHandle, TRUE, INFINITE);
	

	isDoneB = true;
	if (ReleaseSemaphore(fillBC, numThread, NULL) == 0)  //to wake up all thread blocked on waitforsingleobejct
		printf("Error: %x\n", GetLastError());
	
	
	WaitForMultipleObjects(numThread, thCHandle, TRUE, INFINITE);
	

	

	system("PAUSE");

	return 0;
}

//works like producer
DWORD WINAPI ThAWork(LPVOID param) {
	
	THREAD_A_DATA			*data;
	DWORD					timeSleep, index, nIn, selectedQueue;
	HANDLE					file;

	RECORD					record;
	BOOL					canOpen = FALSE;
	DWORD					id;
	RECORD					recordApp;
	DWORD					queue;

	data = (THREAD_A_DATA *)param;

	srand((unsigned)time(NULL));

	id = GetCurrentThreadId();
	

	EnterCriticalSection(data->csFileOpening);
	//file opening making sure to do not open the same file
	while (!canOpen) {

		index = (rand() % data->numFile); //get the index in order to access to the array containing the name of the files
		
		if (data->hashOpenedFileA[index] == 0) { 

			file = CreateFile(data->listOfFileA[index], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
				return -2;
			}
			data->hashOpenedFileA[index] = 1;
			canOpen = true;
		}
	}
	LeaveCriticalSection(data->csFileOpening);
	
	
	//start
	while (ReadFile(file, &record, sizeof(RECORD), &nIn, NULL) > 0 && nIn > 0) {
		
		if (WaitForSingleObject(*data->emptyAB, INFINITE) != WAIT_OBJECT_0)
			return -1;

		EnterCriticalSection(data->csAB);


		timeSleep = (rand() % 10) + 1;
		Sleep(timeSleep *100);
			
		selectedQueue = (rand() % data->numFile);

		//arise non alphabetic character
		int j = 0;
		for ( int i = 0; i < record.N; i++)
			if ((record.sequenceOfCharacter[i] >= 'a' && record.sequenceOfCharacter[i] <= 'z') || (record.sequenceOfCharacter[i] >= 'A' && record.sequenceOfCharacter[i] <= 'Z')) {
				recordApp.sequenceOfCharacter[j] = record.sequenceOfCharacter[i];
				j++;
			}
		recordApp.sequenceOfCharacter[j] = 0; //TERMINATOR charatcetr
		recordApp.N = j;

		//put in the queue the record just generate
		insert(&data->allQueueA[selectedQueue], recordApp);


		_tprintf(_T("Thread A-%d has read %s which will be inserted in queueA-%d\n") , id , record.sequenceOfCharacter , selectedQueue);

		LeaveCriticalSection(data->csAB);
		if (ReleaseSemaphore(*data->fillAB, 1, NULL) == 0)
			return -2;

	}
	
	
	CloseHandle(file);

	return 0;
}


//works as consumer to comunicate to A and as Producer to comunicate to thread D
DWORD WINAPI ThBWork(LPVOID param) {

	THREAD_B_DATA			*data;
	DWORD					selectedQueue, id;
	RECORD					*extractedValue = NULL;

	BOOL					terminate = FALSE;

	data = (THREAD_B_DATA *)param;

	id = GetCurrentThreadId();

	srand((unsigned)time(NULL));
	
	while (true) {
		
		
		if (WaitForSingleObject(*data->fillAB, INFINITE) != WAIT_OBJECT_0)
			return -3;
		

		//stop condition
		if (isDoneA) 
			return 0;	


		EnterCriticalSection(data->csAB);


		// in order to do not read from an empty queue
		while (extractedValue == NULL) {

			selectedQueue = (rand() % data->numFile);
			extractedValue = remove(&data->allQueueA[selectedQueue]);

		}


		_tprintf(_T("Thread B-%d has read from queueA-%d the value : %s\n"), id, selectedQueue, extractedValue->sequenceOfCharacter, extractedValue->N);


		//loweCase letter to upperCase letter
		for (int i = 0; i < extractedValue->N; i++)
			if (extractedValue->sequenceOfCharacter[i] >= 'a' &&  extractedValue->sequenceOfCharacter[i] <= 'z')
				extractedValue->sequenceOfCharacter[i] -= 32;  //32 beacause from 'a' to 'A' there are 32 character

	

		LeaveCriticalSection(data->csAB);
		if (ReleaseSemaphore(*data->emptyAB, 1, NULL) == 0) {

			fprintf(stderr, "Error: %d", GetLastError());
			return -4;
		}
			
	
		
		// B TO C
		if (WaitForSingleObject(*data->emptyBC, INFINITE) != WAIT_OBJECT_0)
			return -5;

		EnterCriticalSection(data->csBC);


		selectedQueue = (rand() % data->numFile);

		//put into the queue B the value generate by changing the lowecase letter in uppercase letter
		insert(&data->allQueueB[selectedQueue], *extractedValue);


		//set extractedValue = NULL in order to do not exceed the next loop
		extractedValue = NULL;

		
		LeaveCriticalSection(data->csBC);
		if (ReleaseSemaphore(*data->fillBC, 1, NULL) == 0)
			return -6;
		

	}

	return 3;
}

//works as consumer 
DWORD WINAPI ThCWork(LPVOID param) {

	THREAD_C_DATA			*data;
	RECORD					*extractedValue = NULL;
	DWORD					id, selectedQueue, index, nOut;
	HANDLE					file;
	BOOL					terminate = FALSE;

	data = (THREAD_C_DATA *)param;

	id = GetCurrentThreadId();

	while (true) {
		

		if (WaitForSingleObject(*data->fillBC, INFINITE) != WAIT_OBJECT_0)
			return -7;
		
		//stop conditon
		if (isDoneB)
			return 0;

		EnterCriticalSection(data->csBC);
		

		// in order to do not read from an empty queue
		while (extractedValue == NULL) {

			selectedQueue = (rand() % data->numFile);
			extractedValue = remove(&data->allQueueB[selectedQueue]);
	
		}
			

		_tprintf(_T("Thread C-%d has read from queueB-%d the value : %s\n"), id, selectedQueue, extractedValue->sequenceOfCharacter, extractedValue->N);

		

		//write the record into the file
		index = (rand() % data->numFile);
		if (data->hashOpenedFileC[index] == 0) {

			file = CreateFile(data->listOfFileB[index], FILE_APPEND_DATA, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
				return -2;
			}

		}

		WriteFile(file, extractedValue, sizeof(RECORD), &nOut, NULL);
		CloseHandle(file);

		
		extractedValue = NULL;
		
			
		LeaveCriticalSection(data->csBC);
		if (ReleaseSemaphore(*data->emptyBC, 1, NULL) == 0)
			return -8;

	
	}

	
	return 4;
}




// queue
VOID init(LIST* ptr, INT size)
{

	ptr->buffer = (RECORD *)malloc(size * sizeof(RECORD));

	ptr->size = size;
	ptr->read = 0;
	ptr->write = 0;

	for (int i = 0; i < size; i++)
		memset(ptr->buffer[i].sequenceOfCharacter, 0, 512);
	
}


RECORD *remove(LIST * ptr)
{
	RECORD			*val;


	if (ptr->read == ptr->write) return NULL;

	val = &ptr->buffer[ptr->read];
	ptr->read = (ptr->read+1)%ptr->size;
	

	return val;
}




DWORD insert(LIST * ptr, RECORD value)
{

	ptr->buffer[ptr->write]= value;
	ptr->write = (ptr->write + 1) % ptr->size;;

	return 0;
}

VOID visit(LIST * ptr)
{
	int pos; //indice posizione 
	for (pos = 0; pos < ptr->size; pos = pos++)
		_tprintf(_T("%s "), ptr->buffer[pos]);

	printf("\n");
}



