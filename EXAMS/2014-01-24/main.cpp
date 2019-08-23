// 2014-01-24.cpp : definisce il punto di ingresso dell'applicazione console.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE

#define		BUFFER					500
#define		CREATE_NOT_SUSPEND			0
	
typedef struct _RECORD_OUTPUT {
	DWORD				t_id;
	DWORD				n_cars;
	DWORD				tot_price;
	TCHAR				data[50];
	TCHAR				branch_name[50];
} RECORD_OUTPUT;

typedef struct _RECORD_PRODUCT {
	TCHAR				manufacter[50];
	TCHAR				car[50];
	TCHAR				file[50];
} RECORD_PRODUCT;

typedef struct _RECORD_SOLD_CAR {
	TCHAR				id[50];
	TCHAR				selling_data[50];
	DWORD				price;
} RECORD_SOLD_CAR;

typedef struct _THREAD_DATA {
	TCHAR				input_file_name[BUFFER];
	TCHAR				output_file_name[BUFFER];
	PDWORD				offset_file_product;
	HANDLE				h_product_file;
	DWORD				N;
	PTCHAR				branch_name_more_sold_car;
	PTCHAR				date_sold_car;
	PCRITICAL_SECTION	cs_get_offset;
	PCRITICAL_SECTION	cs_print;
	PCRITICAL_SECTION   cs_check_parameters;
} THREAD_DATA;

DWORD WINAPI th_work(LPVOID param);
DWORD is_there(PTCHAR data, TCHAR **array_of_data, INT size);
DWORD get_index(PDWORD hash_array, DWORD size);

INT _tmain(INT argc, LPTSTR argv[]){
	//get input parameter
	TCHAR	input_file_name[BUFFER], output_file_name[BUFFER];
	DWORD	N;
	DWORD	offset_file_product = 0;
	TCHAR   branch_name_more_sold_car[50], date_sold_car[50];
	TCHAR	str_name[BUFFER];

	_tcscpy(input_file_name, argv[1]);
	N = _tstoi(argv[2]);
	_tcscpy(output_file_name, argv[3]);

	_tprintf(_T("argv %s %d %s\n"), input_file_name, N, output_file_name);

	//variable for threads
	HANDLE		*h_thread  = (PHANDLE)malloc(N * sizeof(HANDLE));
	DWORD		*id_thread = (PDWORD)malloc(N * sizeof(HANDLE));
	THREAD_DATA *d_thread =  (THREAD_DATA *)malloc(N * sizeof(THREAD_DATA));

	//synchronization object
	CRITICAL_SECTION cs_get_offset, cs_print, cs_check_parameters;
	InitializeCriticalSection(&cs_get_offset);
	InitializeCriticalSection(&cs_print);
	InitializeCriticalSection(&cs_check_parameters);

	for (INT i = 0; i < N; i++) {
		_tcscpy(d_thread[i].input_file_name, input_file_name);
		_tcscpy(d_thread[i].output_file_name, output_file_name);
		d_thread[i].offset_file_product = &offset_file_product;
		d_thread[i].cs_get_offset = &cs_get_offset;
		d_thread[i].N = N;
		d_thread[i].cs_print = &cs_print;
		d_thread[i].branch_name_more_sold_car = branch_name_more_sold_car;
		d_thread[i].date_sold_car = date_sold_car;
		d_thread[i].cs_check_parameters = &cs_check_parameters;

		h_thread[i] = CreateThread(NULL, 0, th_work, &d_thread[i], CREATE_NOT_SUSPEND, &id_thread[i]);
	}

	WaitForMultipleObjects(N, h_thread, TRUE, INFINITE);

	HANDLE			in_file, out_file;
	RECORD_OUTPUT	ro;
	DWORD			nIn, nOut;
	if ((out_file = CreateFile(output_file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "error : %d\n", GetLastError());
		return -7;
	}
	
	for (INT i = 0; i < N; i++) {
		_stprintf(str_name, _T("%dfile.bin"), id_thread[i]);
		in_file = CreateFile(str_name, GENERIC_READ, FILE_SHARE_READ , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		while (ReadFile(in_file, &ro, sizeof(RECORD_OUTPUT), &nIn, NULL) > 0 && nIn > 0)
			WriteFile(out_file, &ro, sizeof(RECORD_OUTPUT), &nOut, NULL);
		CloseHandle(in_file);
	}
	CloseHandle(out_file);

	//print on stdout the result
	if ((out_file = CreateFile(output_file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "error  %d\n", GetLastError());
		return -5;
	}

	memset(&ro, 0, sizeof(RECORD_OUTPUT));
	while ((ReadFile(out_file, &ro, sizeof(RECORD_OUTPUT), &nIn, NULL) > 0 ) && nIn > 0)
		_tprintf(_T("record: %s %d %d %d %s\n"), ro.branch_name , ro.t_id, ro.n_cars, ro.tot_price , ro.data);

	//delete files genereted by the threads
	for (INT i = 0; i < N; i++) {
		_stprintf(str_name, _T("%dfile.bin"), id_thread[i]);
		DeleteFile(str_name);
	}
		
	CloseHandle(out_file);
	system("PAUSE");

	return 0;
}


DWORD WINAPI th_work(LPVOID param) {

	THREAD_DATA		*d = (THREAD_DATA *)param;
	RECORD_PRODUCT	r;
	RECORD_SOLD_CAR rs;
	DWORD			nIn = 0, nOut = 0;
	LARGE_INTEGER  	where_read_lgi;
	DWORD			where_read_dw;
	OVERLAPPED      ov = { 0 , 0 , 0 , 0 , NULL };
	HANDLE			h_trade_file;
	DWORD			tot_sold_cars = 0;
	DWORD			tot_amount = 0;
	RECORD_OUTPUT	ro;
	HANDLE          single_output_file;
	TCHAR			str_name[BUFFER];
	DWORD			index_for_date = 0;
	DWORD			index_for_branch_name = 0;
	DWORD			get_index_data = 0;
	DWORD			get_index_branch_name = 0;
	DWORD			how_many_record = 0;
	TCHAR			**comparing_date, **comparing_branch_name;
	PDWORD			hash_comparing_table_data, hash_comparing_table_branch_name;
	INT				first_time = 0;
	DWORD			choosen_index = 0;

	//open file in which thread will write only own output
	_stprintf(str_name, _T("%dfile.bin"), GetCurrentThreadId());
	single_output_file = CreateFile(str_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	HANDLE		h_product_file = CreateFile(d->input_file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER n = { 0 };
	GetFileSizeEx(h_product_file, &n);
	DWORD num_product = n.QuadPart / sizeof(RECORD_PRODUCT);

	while ( 1 ) {
		//get the offset ( different value for each thread in order to do not read the same line )
		EnterCriticalSection(d->cs_get_offset);
			where_read_dw = *d->offset_file_product;
			*d->offset_file_product += 1;
		LeaveCriticalSection(d->cs_get_offset);
		//stop condition
		if (where_read_dw >= num_product) {
			CloseHandle(single_output_file);
			CloseHandle(h_product_file);
			return 1;

		}

		//OVERLAPPED setting
		where_read_lgi.QuadPart = where_read_dw * sizeof(RECORD_PRODUCT);
		ov.Offset = where_read_lgi.LowPart;
		ov.OffsetHigh = where_read_lgi.HighPart;
		if (ReadFile(h_product_file, &r, sizeof(RECORD_PRODUCT), &nIn, &ov) == 0) {
			fprintf ( stderr , "error %d\n" , GetLastError());
			CloseHandle(single_output_file);
			CloseHandle(h_product_file);
			return -1;
		}

		//open trade file whose name is defined within of the record just read
		h_trade_file = CreateFile(r.file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h_trade_file == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "error :  %d\n", GetLastError());
			CloseHandle(single_output_file);
			CloseHandle(h_product_file);
			return -2;
		}
			
		//get file size in order to get the number of records ( file_size / sizeof ( record ) )
		GetFileSizeEx(h_trade_file, &n);
		how_many_record = n.QuadPart / sizeof(RECORD_SOLD_CAR);

		//D Y N A M I C   A L L O C A T I O N
		//dynamic array for getting the data with in which there have been more sold cars
		if (!first_time) {
			//date
			comparing_date = (PTCHAR *)malloc(how_many_record * sizeof(PTCHAR));
			for (INT i = 0; i < how_many_record; i++) {
				comparing_date[i] = (PTCHAR)malloc(50 * sizeof(TCHAR));
				memset(comparing_date[i], 0, 50 * sizeof(TCHAR));
			}
			hash_comparing_table_data = (PDWORD)malloc(how_many_record * sizeof(DWORD));
			for (int i = 0; i < how_many_record; i++)
				hash_comparing_table_data[i] = 0;

			//branch_name
			comparing_branch_name = (PTCHAR *)malloc(how_many_record * sizeof(PTCHAR));
			for (INT i = 0; i < how_many_record; i++) {
				comparing_branch_name[i] = (PTCHAR)malloc(50 * sizeof(TCHAR));
				memset(comparing_branch_name[i], 0, 50 * sizeof(TCHAR));
			}
			hash_comparing_table_branch_name = (PDWORD)malloc(how_many_record * sizeof(DWORD));
			for (int i = 0; i < how_many_record; i++)
				hash_comparing_table_branch_name[i] = 0;

			first_time = 1;

		}
		else if (first_time == 1) {
			
			//date
			comparing_date = (PTCHAR *)realloc(comparing_date, how_many_record * sizeof(PTCHAR));
			for (INT i = 0; i < how_many_record; i++) {
				comparing_date[i] = (PTCHAR)realloc(comparing_date[i], 50 * sizeof(TCHAR));
				memset(comparing_date[i], 0, 50 * sizeof(TCHAR));
			}
			hash_comparing_table_data = (PDWORD)realloc(hash_comparing_table_data, how_many_record * sizeof(DWORD));
			for (int i = 0; i < how_many_record; i++)
				hash_comparing_table_data[i] = 0;

			//branc_name
			comparing_branch_name = (PTCHAR *)realloc(comparing_branch_name, how_many_record * sizeof(PTCHAR));
			for (INT i = 0; i < how_many_record; i++) {
				comparing_branch_name[i] = (PTCHAR)realloc(comparing_branch_name[i], 50 * sizeof(TCHAR));
				memset(comparing_branch_name[i], 0, 50 * sizeof(TCHAR));
			}
			hash_comparing_table_branch_name = (PDWORD)realloc(hash_comparing_table_branch_name, how_many_record * sizeof(DWORD));
			for (int i = 0; i < how_many_record; i++)
				hash_comparing_table_branch_name[i] = 0;

		}

		//each cycle have to be set to 0
		index_for_date = 0;
		index_for_branch_name = 0;
		get_index_data = 0;
		get_index_branch_name = 0;

		//read within the trade file
		while ((ReadFile(h_trade_file, &rs, sizeof(RECORD_SOLD_CAR), &nIn, NULL) > 0 ) && nIn > 0) {

			tot_amount += rs.price;
			tot_sold_cars += 1;

			//DATA
			get_index_data = is_there(rs.selling_data, comparing_date, how_many_record);
			if (get_index_data == -1) {
				_tcscpy(comparing_date[index_for_date], rs.selling_data);
				hash_comparing_table_data[index_for_date] += 1;
				index_for_date++;
			}
			else
				hash_comparing_table_data[get_index_data] += 1;

			//BRANCH NAME
			get_index_branch_name = is_there(rs.id, comparing_branch_name, how_many_record);
			if (get_index_branch_name == -1) {
				_tcscpy(comparing_branch_name[index_for_branch_name], rs.id);
				hash_comparing_table_branch_name[index_for_branch_name] += 1;
				index_for_branch_name++;
			}
			else
				hash_comparing_table_data[get_index_branch_name] += 1;
		}

		//output record genereted in order to be written on thread-file
		ro.t_id = GetCurrentThreadId();
		ro.tot_price = tot_amount;
		ro.n_cars = tot_sold_cars;

		//get the index where DATA is
		choosen_index = get_index(hash_comparing_table_data, how_many_record);
		_tcscpy(ro.data, comparing_date[choosen_index]);

		//get the index where DATA is
		choosen_index = get_index(hash_comparing_table_branch_name, how_many_record);
		_tcscpy(ro.branch_name, comparing_branch_name[choosen_index]);


		if (WriteFile(single_output_file, &ro, sizeof(RECORD_OUTPUT), &nOut, NULL) == 0) {
			fprintf(stderr, "error :  %d\n", GetLastError());
			CloseHandle(single_output_file);
			CloseHandle(h_product_file);
			return -4;
		}

		CloseHandle(h_trade_file);

		tot_amount = 0;
		tot_sold_cars = 0;
	}

	return 0;
}


DWORD is_there(PTCHAR data, TCHAR **array_of_data , INT size) {
	for (INT i = 0; i < size; i++) {
		if (_tcscmp(data, array_of_data[i]) == 0)
			return i;
	}
	return -1;

}


DWORD get_index ( PDWORD hash_array , DWORD size ){
	DWORD max = 0;
	DWORD index = 0;
	
	for (INT i = 0; i < size; i++) {
		if (hash_array[i] > max) {
			max = hash_array[i];
			index = i;
		}
	}
	return index;
}
