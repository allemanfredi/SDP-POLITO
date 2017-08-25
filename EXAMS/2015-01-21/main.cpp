
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

	TCHAR		data[BUFFER];
	INT			val;

}RECORD;

typedef struct _OUTPUT_RECORD {

	INT	    val[2];
	INT   	month;


}OUTPUT_RECORD;


typedef struct _THREAD_DATA {

	INT					N;
	INT					M;
	PTCHAR				output_file_name;

	INT					bank_account;

	PHANDLE				sem_finish_calculating;
	PHANDLE				sem_can_go_ahead;
	PCRITICAL_SECTION	is_first;

	PINT				finish_reading;

}THREAD_DATA;


//function prototype
DWORD WINAPI th_work(LPVOID param);
void check_output_file_correctness(PTCHAR file_name, INT N);

INT _tmain(INT argc, LPTSTR argv[])
{
	INT			N, M;
	TCHAR		output_file_name[BUFFER];

	PHANDLE		h_thread;
	PDWORD		id_thread;

	INT			finish_reading = 0;

	N = _tstoi(argv[1]);
	M = _tstoi(argv[2]);
	_tcscpy(output_file_name, argv[3]);


	//thread information
	THREAD_DATA *t_data = (THREAD_DATA *)malloc(N * sizeof(THREAD_DATA));
	h_thread = (PHANDLE)malloc(N * sizeof(HANDLE));
	id_thread = (PDWORD)malloc(N * sizeof(DWORD));

	//synchronization object
	PHANDLE	sem_finish_calculating = (PHANDLE)malloc(N * sizeof(HANDLE)); 
	PHANDLE	sem_can_go_ahead = (PHANDLE)malloc(N * sizeof(HANDLE));
	CRITICAL_SECTION is_first;


	//synchronization object initialzation
	for (int i = 0; i < N; i++) {
		sem_finish_calculating[i] = CreateSemaphore(NULL, 0, N, NULL);
		sem_can_go_ahead[i] = CreateSemaphore(NULL, 0, N, NULL);
	}
	
	InitializeCriticalSection(&is_first);


	for (INT i = 0; i < N; i++) {

		t_data[i].M = M;
		t_data[i].N = N;
		t_data[i].output_file_name = output_file_name;
		t_data[i].bank_account = i;
		t_data[i].is_first = &is_first;
		t_data[i].sem_finish_calculating = sem_finish_calculating;
		t_data[i].finish_reading = &finish_reading;
		t_data[i].sem_can_go_ahead = sem_can_go_ahead;


		h_thread[i] = CreateThread(NULL, 0, th_work, &t_data[i], CREATE_NOT_SUSPEND, &id_thread[i]);

	}


	WaitForMultipleObjects(N, h_thread, TRUE, INFINITE);

	check_output_file_correctness(output_file_name, N);

	system("PAUSE");

	return 0;
}


DWORD WINAPI th_work(LPVOID param) {

	THREAD_DATA		*d = (THREAD_DATA *)param;
	INT				current_month = 0;
	HANDLE			file, output_file;
	TCHAR			file_name[BUFFER];
	RECORD			record;
	DWORD			nIn = 0;
	INT				monthly_amount = 0;
	INT				im_first = 0;
	INT				loop = 0;

	OVERLAPPED ov = { 0 , 0 , 0 , NULL };
	LARGE_INTEGER month_pos, account_pos;

	TCHAR			buf[BUFFER];
	DWORD			nOut;


	while (loop < d->M) {

		memset(file_name, 0, BUFFER);
		//GET BALANCE
		_stprintf(file_name, _T("account%dmonth%d.bin"), d->bank_account + 1, loop + 1);
		file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


		while (ReadFile(file, &record, sizeof(RECORD), &nIn, NULL) > 0 && nIn > 0)
			monthly_amount += record.val;

		CloseHandle(file);

		//FILE
		//here i have to properly write on the file (  thread that manage account N 1, also write the month number
		output_file = CreateFile(d->output_file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);


		if (d->bank_account == 0) {

			month_pos.QuadPart = (d->N + 1) * loop * sizeof(INT);
			ov.Offset = month_pos.LowPart;
			ov.OffsetHigh = month_pos.HighPart;
			WriteFile(output_file, &loop , sizeof(INT), &nOut, &ov);
		}

		account_pos.QuadPart = (loop * (d->N + 1) + (d->bank_account + 1)) * sizeof(INT);
		ov.Offset = account_pos.LowPart;
		ov.OffsetHigh = account_pos.HighPart;
		WriteFile(output_file, &monthly_amount, sizeof(int), &nOut, &ov);

		CloseHandle(output_file);


		//B A R R I E R 
		//the first thread that ends has to wait the others
		EnterCriticalSection(d->is_first);
		if (*d->finish_reading == 0) {
			im_first = 1;
			*d->finish_reading = 1;
		}
		else
			im_first = 0;
		LeaveCriticalSection(d->is_first);

		if (im_first == 1) {

			for (int i = 0; i < d->N; i++) //wait all object but the last one
				if ( i != d->bank_account )
					WaitForSingleObject(d->sem_finish_calculating[i], INFINITE);

			*d->finish_reading = 0;

			//wake up waiting thread 
			for (int i = 0; i < d->N; i++)
				if (d->bank_account != i) {
					if (ReleaseSemaphore(d->sem_can_go_ahead[i], 1, NULL) == 0) {
						fprintf(stderr, "1 Error : %d\n", GetLastError());
						return -1;
					}
				}

		}

		else if (im_first == 0) {
			if (ReleaseSemaphore(d->sem_finish_calculating[d->bank_account], 1, NULL) == 0) {
				fprintf(stderr, "2 Error : %d\n", GetLastError());
				return -2;
			}
			WaitForSingleObject(d->sem_can_go_ahead[d->bank_account], INFINITE);
		}

		loop+=1;
		im_first = 0;

	}
	return 0;
}



void check_output_file_correctness(PTCHAR file_name , INT N) {

	OUTPUT_RECORD	record;
	INT				n;
	DWORD			nIn;

	HANDLE file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	while (ReadFile(file, &n, sizeof(INT), &nIn, NULL) > 0 && nIn > 0) 
		_tprintf(_T("%d "), n);

	_tprintf(_T("\n"));
	CloseHandle(file);

	return;
}
