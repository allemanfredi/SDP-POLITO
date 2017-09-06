#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>


#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE


#define		BUFFER					500
#define		CREATE_NOT_SUSPEND			0
#define		SIZE_QUEUE				10

typedef struct _RECORD {

	TCHAR			name[40];
	INT			C;
	INT			id;
	FLOAT			predicted_duration;

}RECORD;


typedef struct _QUEUE {
	
	RECORD			*buffer;
	INT		  	size;
	INT		   	read;
	INT		   	write;

	INT		   	max_predicted_time;
	INT		   	min_predicted_time;

}QUEUE;

typedef struct _DATA_CLIENT {

	INT		   	M;
	INT			Q;
	INT			N;

	INT			C;

	QUEUE			*queues;

	TCHAR			file_name[BUFFER];

	PHANDLE			full;
	PHANDLE			empty;
	LPCRITICAL_SECTION	cs_client;

}DATA_CLIENT;


typedef struct _DATA_SERVER {

	INT			M;
	INT			Q;
	INT			N;

	QUEUE			*queues;

	PHANDLE			full;
	PHANDLE			empty;
	LPCRITICAL_SECTION	cs_server;

}DATA_SERVER;


BOOL is_done = FALSE;


//function prototype
DWORD insert(QUEUE * ptr, RECORD value);
VOID init(QUEUE* ptr, INT size, INT min_predicted_time, INT max_predicted_time);
RECORD remove(QUEUE * ptr);

DWORD select_queue(QUEUE *queues, INT num_queues, INT duration);

DWORD WINAPI client_work(LPVOID param);
DWORD WINAPI server_work(LPVOID param);

INT _tmain(INT argc, LPTSTR argv[]){

	TCHAR			file_name[BUFFER];
	INT			M, N, Q;
	DWORD			n_in;
	HANDLE			h_in_file;


	//get input parameter
	_tcscpy(file_name, argv[1]);

	
	//open the file in which there are the queue settings
	h_in_file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


	//get M , N , and Q
	ReadFile(h_in_file, &M, sizeof(INT), &n_in, NULL);
	ReadFile(h_in_file, &N, sizeof(INT), &n_in, NULL);
	ReadFile(h_in_file, &Q, sizeof(INT), &n_in, NULL);



	//dynamic allocation of 2*q array
	FLOAT *predicted_time = (FLOAT *)malloc(2 * Q * sizeof(FLOAT));
	

	
	//get 2q array which contains two numbers represent the minimum and the maximum 
	//predicted duration time for the tasks to be en - queued.
	for (INT i = 0; i < (2 * Q); i++)
		ReadFile(h_in_file, &predicted_time[i], sizeof(FLOAT), &n_in, NULL);


	
	//queue initialization
	QUEUE *queues = (QUEUE *)malloc(Q * sizeof(QUEUE));
	for (int j , i = j = 0; i < 2 * Q; i += 2, j++)
		init(&queues[j], SIZE_QUEUE, predicted_time[i], predicted_time[i+1]);
	


	//thread variable
	HANDLE	*h_th_client = (HANDLE *)malloc(N * sizeof(HANDLE));
	HANDLE	*h_th_server = (HANDLE *)malloc(M * sizeof(HANDLE));

	DWORD   *id_th_client = (DWORD *)malloc(N * sizeof(DWORD));
	DWORD   *id_th_server = (DWORD *)malloc(M * sizeof(DWORD));

	DATA_CLIENT *d_client = (DATA_CLIENT *)malloc(N * sizeof (DATA_CLIENT));
	DATA_SERVER *d_server= (DATA_SERVER *)malloc(N * sizeof(DATA_SERVER));


	
	//synchronization object
	PHANDLE		full  = (PHANDLE)malloc(Q   * sizeof(HANDLE)); /*+1 because the last one is used in order to termianate the thread
																	/*if it is released it means that the server thread has to stop*/	
	PHANDLE		empty = (PHANDLE)malloc(Q * sizeof(HANDLE));
	for (INT i = 0; i < Q; i++) {
		full[i]  = CreateSemaphore(NULL, 0, SIZE_QUEUE, NULL);
		empty[i] = CreateSemaphore(NULL, SIZE_QUEUE, SIZE_QUEUE, NULL);
	}
	
	LPCRITICAL_SECTION cs_client = (LPCRITICAL_SECTION)malloc(Q * sizeof(CRITICAL_SECTION));
	LPCRITICAL_SECTION cs_server = (LPCRITICAL_SECTION)malloc(Q * sizeof(CRITICAL_SECTION));
	for (INT i = 0; i < Q; i++) {
		InitializeCriticalSection(&cs_client[i]);
		InitializeCriticalSection(&cs_server[i]);
	}

	
	
	//close file because each thread will open its own file
	CloseHandle(h_in_file);


	
	//server thread creation
	for (INT i = 0; i < M; i++) {

		d_server[i].full		= full;
		d_server[i].empty		= empty;
		d_server[i].queues		= queues;
		d_server[i].cs_server		= cs_server;
		d_server[i].M			= M;
		d_server[i].N			= N;
		d_server[i].Q			= Q;

		h_th_server[i] = CreateThread(NULL, 0, server_work, &d_server[i], CREATE_NOT_SUSPEND, &id_th_server[i]);
	}


	
	//client thread creation
	for (INT i = 0; i < N; i++) {

		_tcscpy(d_client[i].file_name, file_name);
		d_client[i].full		= full;
		d_client[i].empty		= empty;
		d_client[i].C			= i;
		d_client[i].queues		= queues;
		d_client[i].cs_client		= cs_client;
		d_client[i].M			= M;
		d_client[i].N			= N;
		d_client[i].Q			= Q;

		h_th_client[i] = CreateThread(NULL, 0, client_work, &d_client[i], CREATE_NOT_SUSPEND, &id_th_client[i]);
	}

	
	
	//waiting for client-threads termination
	WaitForMultipleObjects(N, h_th_client, TRUE, INFINITE);


	
	//stopping condition
	is_done = TRUE;
	
	
	
	//here i have to unclock all server thread locked on "full" semaphore 
	for ( int i = 0; i < M; i++)
		ReleaseSemaphore(full[0], 1, 0);


	
	//waiting for server-threads termination
	WaitForMultipleObjects(M, h_th_server, TRUE, INFINITE);

	system("pause");
    return 0;
}




//client
DWORD WINAPI client_work(LPVOID param) {

	DATA_CLIENT *d = (DATA_CLIENT *)param;

	HANDLE				h_in_file;
	DWORD				nIn, nOut;
	OVERLAPPED			overlapped = { 0, 0, 0, 0, NULL };
	LARGE_INTEGER			file_pos;
	LARGE_INTEGER			file_reserved;
	RECORD				r;
	BOOL				first_time = FALSE;


	h_in_file = CreateFile(d->file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	
	while (1) {

		memset(&r, 0, sizeof(RECORD));

		//get the file position in which we want read and lock
		if (!first_time) {

			//loking generation;
			file_pos.QuadPart = ((3 * sizeof(INT)) + (2 * d->Q * sizeof(FLOAT)));//set the position on the file...																	//jump the first 2 lines
			file_reserved.QuadPart = sizeof(RECORD);					  
			first_time = TRUE;
		}
		else {
			file_pos.QuadPart += sizeof(RECORD);
			file_reserved.QuadPart = sizeof(RECORD);
		}
		overlapped.Offset = file_pos.LowPart;
		overlapped.OffsetHigh = file_pos.HighPart;

		

		if (LockFileEx(h_in_file, LOCKFILE_EXCLUSIVE_LOCK, 0, file_reserved.LowPart, file_reserved.HighPart, &overlapped) == FALSE) {
			_ftprintf(stderr, _T("error 1 : %d\n"), GetLastError());
			exit(EXIT_FAILURE);
		}


		if ((ReadFile(h_in_file, &r, sizeof(RECORD), &nIn, &overlapped) == 0) && nIn == 0) {
			//read EOF -----> unlock file and stop 
			UnlockFileEx(h_in_file, 0, file_reserved.LowPart, file_reserved.HighPart, &overlapped);
			return -1;
		}
		
		
		if (UnlockFileEx(h_in_file, 0, file_reserved.LowPart, file_reserved.HighPart, &overlapped) == FALSE) {
			_ftprintf(stderr, _T("error 2 : %d\n"), GetLastError());
			exit(EXIT_FAILURE);
		}


		//here i have to enqueue the task in the proper queue
		if (r.C == d->C) { //same C value -----> client can enqueue the task
			//SYNCHRONIZATION POINT ( p&c )
			INT index_choosen_queue = select_queue(d->queues, d->Q, r.predicted_duration);
			if (index_choosen_queue == -1)
				continue;
			WaitForSingleObject(d->empty[index_choosen_queue], INFINITE);
				EnterCriticalSection(&d->cs_client[index_choosen_queue]);
					insert(&d->queues[index_choosen_queue], r);
				LeaveCriticalSection(&d->cs_client[index_choosen_queue]);
			ReleaseSemaphore(d->full[index_choosen_queue], 1, 0);
		
		}
	}
	
	return 0;
}




//server
DWORD WINAPI server_work(LPVOID param) {

	DATA_SERVER 	*d = (DATA_SERVER *)param;
	RECORD		r;
	DWORD		res = 0;

	while (1) {

		res = WaitForMultipleObjects(d->Q, d->full, FALSE, INFINITE); //waitfor multipleobject
			res -= WAIT_OBJECT_0;
			//stopping conditon
			if (is_done)
				return 1;

			EnterCriticalSection(&d->cs_server[res]);
				r = remove(&d->queues[res]);
				_tprintf(_T("(%d) is running %s \n"), GetCurrentThreadId(), r.name);
				Sleep(r.predicted_duration);
			LeaveCriticalSection(&d->cs_server[res]);
		ReleaseSemaphore(d->empty[res] , 1 , 0);
	}
	
	return 0;
}




DWORD select_queue(QUEUE *queues, INT num_queues , INT duration ) {

	for (INT i = 0; i < num_queues; i++) 
		if ((queues[i].min_predicted_time < duration / 100 ) && (queues[i].max_predicted_time > duration / 100))
			return i;

	return -1;
}





//QUEUE functions
VOID init(QUEUE* ptr, INT size , INT min_predicted_time , INT max_predicted_time){

	ptr->buffer = (RECORD *)malloc(size * sizeof(RECORD));
	ptr->size = size;
	ptr->read = 0;
	ptr->write = 0;
	ptr->max_predicted_time = max_predicted_time;
	ptr->min_predicted_time = min_predicted_time;

}




RECORD remove(QUEUE * ptr) {
	
	RECORD			val;

	val = ptr->buffer[ptr->read];
	ptr->read++;
	if (ptr->read > ptr->size - 1)
		ptr->read = 0;

	return val;
}




DWORD insert(QUEUE * ptr, RECORD value){

	ptr->buffer[ptr->write] = value;
	ptr->write++;
	if (ptr->write > ptr->size - 1)
		ptr->write = 0;

	return 0;
}

