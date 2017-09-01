// 2013-02-06.cpp : definisce il punto di ingresso dell'applicazione console.
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
#define		TYPE_FILE				1
#define		TYPE_DIR				2
#define		TYPE_DOT				3


typedef struct _THREAD_DATA {

	TCHAR						source[BUFFER];
	TCHAR						destination[BUFFER];

	PTCHAR						name_for_merge;

	LPCRITICAL_SECTION				cs_check_file_presence;
	LPCRITICAL_SECTION				cs_set_file_name_for_merge;
	PHANDLE						sem_finish;
	PHANDLE						sem_go_ahead;

	DWORD						which_semaphore;

}THREAD_DATA;

typedef struct _THREAD_DATA_MERGE {

	DWORD						N;
	PTCHAR						name_for_merge;

	PHANDLE						sem_finish;
	PHANDLE						sem_go_ahead;

}THREAD_DATA_MERGE;


BOOL is_done = FALSE;


//function prototype
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
BOOL DirectoryExists(LPCTSTR szPath);
INT	get_number_of_word(PTCHAR line);
DWORD WINAPI th_work(LPVOID param);
DWORD WINAPI th_merge(LPVOID param);

INT _tmain(INT argc, LPTSTR argv[])
{
	DWORD						N;
	TCHAR						source[BUFFER];
	TCHAR						destination[BUFFER];
	TCHAR						name_for_merge[BUFFER];


	//get input parameter
	N = _tstoi(argv[1]);
	_tcscpy(source, argv[2]);
	_tcscpy(destination, argv[3]);

	//variable for threads
	THREAD_DATA *d_thread  = (THREAD_DATA *)malloc(N * sizeof(THREAD_DATA));
	HANDLE      *h_thread  = (HANDLE *)malloc(N * sizeof(HANDLE));
	DWORD		*id_thread = (DWORD *)malloc(N * sizeof(HANDLE));
	HANDLE		h_merge_thread;
	DWORD		id_thread_merge;


	//synchronizaton object
	HANDLE		*sem_finish = (HANDLE *) malloc(N * sizeof(HANDLE));
	HANDLE		 *sem_go_ahead = (HANDLE *)malloc(N * sizeof(HANDLE));
	for (INT i = 0; i < N; i++)
		sem_finish[i] = CreateSemaphore(NULL, 0, N , NULL);
	for (INT i = 0; i < N; i++)
		sem_go_ahead[i] = CreateSemaphore(NULL, 0, N, NULL);
	
	CRITICAL_SECTION		cs_check_file_presence, cs_set_file_name_for_merge;
	InitializeCriticalSection(&cs_check_file_presence);
	InitializeCriticalSection(&cs_set_file_name_for_merge);

	//normal thread
	for (INT i = 0; i < N; i++) {

		_tcscpy(d_thread[i].source, source);
		_tcscpy(d_thread[i].destination, destination);
		d_thread[i].cs_check_file_presence = &cs_check_file_presence;
		d_thread[i].cs_set_file_name_for_merge = &cs_set_file_name_for_merge;
		d_thread[i].name_for_merge = name_for_merge;
		d_thread[i].sem_finish = sem_finish;
		d_thread[i].sem_go_ahead = sem_go_ahead;
		d_thread[i].which_semaphore = i;

		h_thread[i] = CreateThread(NULL, NULL, th_work, &d_thread[i], CREATE_NOT_SUSPEND, &id_thread[i]);
	}

	//merge thread

	THREAD_DATA_MERGE d_merge;
	d_merge.sem_finish = sem_finish;
	d_merge.name_for_merge = name_for_merge;
	d_merge.N = N;
	d_merge.sem_go_ahead = sem_go_ahead;

	h_merge_thread = CreateThread(NULL, NULL, th_merge, &d_merge, CREATE_NOT_SUSPEND, &id_thread_merge);

	WaitForMultipleObjects(N, h_thread, TRUE, INFINITE);

	//merge thread has to terminate
	is_done = TRUE;

	//select randomnly a semaphore in order to terminate the merge-thread
	DWORD	to_stop = rand() %N;
	ReleaseSemaphore(sem_finish[to_stop], 1, NULL);
	WaitForSingleObject(h_merge_thread, INFINITE);
	
	
	system("PAUSE");

	return 0;
}

DWORD WINAPI th_merge(LPVOID param) {

	THREAD_DATA_MERGE		*d = (THREAD_DATA_MERGE *)param;
	DWORD				res;
	PHANDLE				app_sem_finish = (PHANDLE)malloc(d->N * sizeof(HANDLE));
	HANDLE				file_to_open, final_file;
	FILE				*file_to_open_f, *final_file_f;
	DWORD				n1, n2, n3, n4, n5, n6;
	TCHAR				ch;
	TCHAR				name_for_merge[BUFFER];

	n1 = n2 = n3 = n4 = n5 = n6 = 0;

	//check the existance of the output file in negative case i have to create if;
	GetFileAttributes(_T("data.txt")); // from winbase.h
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(_T("data.txt")) && GetLastError() == ERROR_FILE_NOT_FOUND) {
		final_file = CreateFile(_T("data.txt"), GENERIC_ALL, NULL, NULL , CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL, NULL);
		CloseHandle(final_file);
	}
	//else // i have to open it in append mode
		

	while (1) {
		
		res = WaitForMultipleObjects(d->N , d->sem_finish  , FALSE , INFINITE);
		
		//stopping condition
		if (is_done)
			return 0;

		//get the index in order to release the right thread
		res -= WAIT_OBJECT_0;

		//get the name because another threac may be modify it
		_tcscpy(name_for_merge, d->name_for_merge);

		//unlock the waiting thread
		ReleaseSemaphore( d->sem_go_ahead[res], 1, NULL);



		final_file_f	= _tfopen(_T("data.txt"), _T("r"));
		file_to_open_f	= _tfopen(name_for_merge ,  _T("r"));

		//get the first line in each file
		_ftscanf(file_to_open_f, _T("%d %d %d\n"), &n1, &n2, &n3); //#line #character #word 
		_ftscanf(final_file_f, _T("%d %d %d\n"), &n4, &n5, &n6); //#line #character #word 


		//calculate the new value
		n4 += n1; n5 += n2; n6 += n3;

		
		fclose(final_file_f);
		final_file_f = _tfopen(_T("data.txt"), _T("r+"));
		//rewind(final_file_f);
		_ftprintf(final_file_f, _T("%d %d %d\n"), n4, n5, n6);

		
		//close
		fclose(file_to_open_f);
		fclose(final_file_f);

		
		//copy the content of the file
		final_file_f   = _tfopen(_T("data.txt"), _T("a"));
		file_to_open_f = _tfopen(name_for_merge, _T("r"));

		//read the first line in order to jump it
		_ftscanf(file_to_open_f, _T("%d %d %d\n"), &n1, &n2, &n3); //#line #character #word
		while ((_ftscanf(file_to_open_f, _T("%c"), &ch)) != EOF) {
			_ftprintf(final_file_f, _T("%c"), ch);
			//_tprintf(_T("%c"), ch);
		}

		fclose(file_to_open_f);
		fclose(final_file_f);

	}


	return 0;
}





DWORD WINAPI th_work (LPVOID param)
{
	THREAD_DATA 			*d = (THREAD_DATA *)param;
	
	DWORD				FType, i;
	HANDLE				search_handle;
	WIN32_FIND_DATA			find_data;
	TCHAR				originalPath[BUFFER] = { 0 };
	TCHAR				allNames[BUFFER] = { 0 };
	THREAD_DATA			d_app;
	BOOL			    	end = TRUE;
	TCHAR				str_name[BUFFER];
	TCHAR				buffer[BUFFER];
	TCHAR				ch;
	DWORD				number_of_lines = 0;
	DWORD				number_of_character = 0;
	DWORD				number_of_words = 0;
	TCHAR				buffer_app[BUFFER];
	FILE				*file_to_copy, *file_to_be_copied;


	//used for recursion
	_tcscpy(originalPath, d->source);

	//absolute path genearatio
	_tcscpy(allNames, d->source);
	_tcscat(allNames, _T("\\*"));

	search_handle = FindFirstFile(allNames, &find_data);



	do {

		FType = FileType(&find_data);
		if (FType == TYPE_FILE) {

			//check the presence of the file in mutual exclusion
			//check if that file has already been copied....in negative case i have to create iT
			EnterCriticalSection(d->cs_check_file_presence);
			_stprintf(str_name, _T("%s\\%s"), d->destination,find_data.cFileName );
			
			GetFileAttributes(str_name); // from winbase.h
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(str_name) && GetLastError() == ERROR_FILE_NOT_FOUND){
				
				//create file in order to be seen from other processes
				file_to_be_copied = _tfopen(str_name, _T("w"));
				fclose(file_to_be_copied);
				LeaveCriticalSection(d->cs_check_file_presence);

		
				_stprintf(str_name, _T("%s\\%s"), d->source, find_data.cFileName);

				//set to 0
				memset(buffer, 0, BUFFER);
				memset(buffer_app, 0, BUFFER);


				//file to copy
				file_to_copy = _tfopen(str_name, _T("r"));
				while ((_ftscanf(file_to_copy, _T("%c"), &ch)) != EOF) { 

					number_of_character++;
					if (ch == '\n') {
						number_of_words += get_number_of_word(buffer);
						number_of_lines++;
						memset(buffer, 0, BUFFER);
						memset(buffer_app, 0, BUFFER);
					}
					else {
						_stprintf(buffer_app, _T("%c"), ch);
						_tcscat(buffer, buffer_app);
					}
				}

				fclose(file_to_copy);

				//open again the file in order to copy every chcracter
				file_to_copy = _tfopen(str_name, _T("r"));

				
				//if file has not already been opened i have to perform the copy in destionation folder
				_stprintf(str_name, _T("%s\\%s"), d->destination, find_data.cFileName);
				file_to_be_copied = _tfopen(str_name, _T("w"));

				
				//copy the file with the first line writte as text asks
				_ftprintf(file_to_be_copied, _T("%d %d %d\n"), number_of_character, number_of_lines, number_of_words);
				while ((_ftscanf(file_to_copy, _T("%c"), &ch)) != EOF)
					_ftprintf(file_to_be_copied, _T("%c"), ch);
				

				fclose(file_to_be_copied);
				fclose(file_to_copy);

				// S Y N C H R O N Y Z A T I O N 
				//here i have to do the synchronzation with the merge-thread
				EnterCriticalSection(d->cs_set_file_name_for_merge);
					_tcscpy(d->name_for_merge, str_name);
					ReleaseSemaphore(d->sem_finish[d->which_semaphore], 1, NULL);
					WaitForSingleObject(d->sem_go_ahead[d->which_semaphore], INFINITE);
				LeaveCriticalSection(d->cs_set_file_name_for_merge);


				number_of_character = 0;
				number_of_lines = 0;
				number_of_words = 0;

			}
			else {
				LeaveCriticalSection(d->cs_check_file_presence);
				//_tprintf(_T("(%d) File has already been opened %s \n"), GetCurrentThreadId() , str_name);
				//CloseHandle(check_file_presence);
			}
			memset(str_name, 0, BUFFER);

		}

		if (FType == TYPE_DIR) {
			//_tprintf(_T("folder %s\n"), find_data.cFileName);


			//new structure to be passed to the next directory-level-tree
			_tcscpy(d_app.source, originalPath);
			_tcscat(d_app.source, _T("\\"));
			_tcscat(d_app.source, find_data.cFileName);

			_tcscpy(d_app.destination, d->destination);
			_tcscat(d_app.destination, _T("\\"));
			_tcscat(d_app.destination, find_data.cFileName);

			//here i have to check if the directory already exists
			if (!DirectoryExists(d_app.destination)) 
				CreateDirectory(d_app.destination, NULL);

			//copy also the critical section
			d_app.cs_check_file_presence = d->cs_check_file_presence;
			d_app.cs_set_file_name_for_merge = d->cs_set_file_name_for_merge;
			d_app.name_for_merge = d->name_for_merge;
			d_app.sem_go_ahead = d->sem_go_ahead;
			d_app.sem_finish = d->sem_finish;
			d_app.which_semaphore = d->which_semaphore;
			
			
			//recursevely call
			th_work(&d_app);
		}


	} while (FindNextFile(search_handle, &find_data));

	FindClose(search_handle);

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



BOOL DirectoryExists(LPCTSTR szPath){
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


INT get_number_of_word(PTCHAR line) {

	INT			number_of_character = _tcslen(line);
	BOOL		is_word = FALSE;
	BOOL		is_word_started = FALSE;
	DWORD		number_of_word = 0;

	for (int i = 0; i < number_of_character; i++ ) {

		if (line[i] != ' ') {
			is_word = TRUE;
			if (is_word_started == FALSE) {
				is_word_started = TRUE;
				number_of_word++;
			}
		}
		if (line[i] == ' ') {
			is_word_started = FALSE;
			is_word = FALSE;
		}

	}

	return number_of_word;

}

