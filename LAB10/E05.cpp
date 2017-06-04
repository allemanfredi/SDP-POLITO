
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE


#define		BUFFER					500
#define		CREATE_NOT_SUSPEND		0

typedef struct _LIST {
	INT		    *buffer;
	INT			 size;
	INT			 read;
	INT			 write;

}LIST;

typedef struct _THREAD_DATA {

	LIST				*list;
	
	DWORD				T;
	PHANDLE				fillCount;
	PHANDLE				emptyCount;

	PCRITICAL_SECTION   cs;

}THREAD_DATA;










//function protoype
DWORD WINAPI Producer(LPVOID param);
DWORD WINAPI Consumer(LPVOID param);
VOID init(LIST* ptr, INT size);
DWORD insert(LIST * ptr, INT value);
VOID visit(LIST * ptr);
DWORD remove(LIST * ptr);

INT _tmain(INT argc, LPTSTR argv[])
{

	DWORD				numProducers;
	DWORD				numConsumers;
	DWORD				T;
	DWORD				circularBufferSize;

	PHANDLE				producersThreadHandle;
	PHANDLE				consumersThreadHandle;
	PDWORD				producersThreadID;
	PDWORD				consumersThreadID;

	LIST				circularBuffer;

	THREAD_DATA			*producersData;
	THREAD_DATA			*consumersData;

	HANDLE				fillCount;
	HANDLE				emptyCount;

	CRITICAL_SECTION	csP , csC;


	srand((unsigned)time(NULL));


	if (argc != 5) {
		fprintf(stderr, "usage <Consumers> <Producers> <Second> <BufferSize>\n");
		return -1;
	}
	
	//getting the paramenter from command line
	numProducers = _tstoi(argv[1]);
	numConsumers = _tstoi(argv[2]);
	T = _tstoi(argv[3]);
	circularBufferSize = _tstoi(argv[4]);

	
	//circular buffer allocation
	init(&circularBuffer, circularBufferSize);

	
	//data structure and vector of id allocation both for producers and for thconsumer
	producersData = (THREAD_DATA *)malloc(numProducers * sizeof(THREAD_DATA));
	consumersData = (THREAD_DATA *)malloc(numConsumers * sizeof(THREAD_DATA)); 
	producersThreadID = (PDWORD)malloc(numProducers * sizeof(DWORD));
	consumersThreadID = (PDWORD)malloc(numConsumers * sizeof(DWORD));


	//array of handle both for the consumers and for the producers
	producersThreadHandle = (PHANDLE)malloc(numProducers * sizeof(HANDLE));
	consumersThreadHandle = (PHANDLE)malloc(numConsumers * sizeof(HANDLE));


	//semaphore and critical section initialization
	fillCount = CreateSemaphore(NULL, 0, circularBufferSize, NULL);
	emptyCount = CreateSemaphore(NULL, circularBufferSize, circularBufferSize, NULL);

	InitializeCriticalSection(&csP);
	InitializeCriticalSection(&csC);

	
	// producer threads creation
	for (int i = 0; i < numProducers; i++) {

		producersData[i].list = &circularBuffer;
		producersData[i].T = T;

		producersData[i].cs = &csP;
		producersData[i].emptyCount = &emptyCount;
		producersData[i].fillCount = &fillCount;

		producersThreadHandle[i] = CreateThread(NULL, 0, Producer, &producersData[i], CREATE_NOT_SUSPEND, &producersThreadID[i]);
	}

	// consumer threads creation
	for (int i = 0; i < numConsumers; i++) {

		consumersData[i].list = &circularBuffer;
		consumersData[i].T = T;

		consumersData[i].cs = &csC;
		consumersData[i].emptyCount = &emptyCount;
		consumersData[i].fillCount = &fillCount;

		consumersThreadHandle[i] = CreateThread(NULL, 0, Consumer, &consumersData[i], CREATE_NOT_SUSPEND, &consumersThreadID[i]);
	}


	WaitForMultipleObjects(numConsumers, consumersThreadHandle, TRUE, INFINITE);

	WaitForMultipleObjects(numProducers, producersThreadHandle, TRUE, INFINITE);

	system("PAUSE");

	return 0;
}


// P R O D U C E R 
DWORD WINAPI Producer(LPVOID param) {

	THREAD_DATA			*data;
	DWORD				generatedValue, timeSleep;

	data = (THREAD_DATA *)param;
	

	srand((unsigned)time(NULL));
	
	while (true) {

		
		WaitForSingleObject(*data->emptyCount, INFINITE);

			EnterCriticalSection(data->cs);


				generatedValue = rand() %100;
				timeSleep = rand() % data->T + 1;
				Sleep(timeSleep*1000);
				insert(data->list, generatedValue);
				

			LeaveCriticalSection(data->cs);

		ReleaseSemaphore(*data->fillCount, 1, NULL);


	}



	return 0;
}


// C O N S U M E R
DWORD WINAPI Consumer(LPVOID param) {
	
	THREAD_DATA			*data;
	DWORD				timeSleep, extractValue;
	DWORD				id;


	id = GetCurrentThreadId();

	data = (THREAD_DATA *)param;
	

	srand((unsigned)time(NULL));

	while (true) {

		

		WaitForSingleObject(*data->fillCount, INFINITE);
			EnterCriticalSection(data->cs);
	
				timeSleep = rand() % data->T + 1;
				Sleep(timeSleep*1000);
				extractValue = remove(data->list);

			LeaveCriticalSection(data->cs);
		ReleaseSemaphore(*data->emptyCount, 1, NULL);
			// print item

		_tprintf(_T("ID: %d ----->  extracted value : %d\n"), id, extractValue);
		visit(data->list);

		

	}


	return 0;
}



//inizializzazione struttura
VOID init(LIST* ptr, INT size)
{
	ptr->buffer = (INT *)malloc(size * sizeof(INT));
	ptr->size = size;
	ptr->read = 0;
	ptr->write = 0;

	for (int i = 0; i < ptr->size; i++)
		ptr->buffer[i] = 0;
}


DWORD remove(LIST * ptr)
{
	INT			val;


	val = ptr->buffer[ptr->read];
	ptr->read++;
	if (ptr->read > ptr->size - 1)
		ptr->read = 0;
	
	return val;
}




DWORD insert(LIST * ptr, INT value)
{
	ptr->buffer[ptr->write] = value;
	ptr->write++;
	if (ptr->write > ptr->size - 1)
		ptr->write = 0;

	return 0;
}

VOID visit(LIST * ptr)
{
	int pos; //indice posizione 
	for(pos=0; pos < ptr->size; pos = pos++) 
		printf("%d ",ptr->buffer[pos]);

	printf("\n");
}

