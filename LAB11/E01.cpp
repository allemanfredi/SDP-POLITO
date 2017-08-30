#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE


#define		BUFFER					300

#define		CREATE_NOT_SUSPEND			0

#define     	IP_SIZE					20
#define		NAME_SIZE				20
#define		DATE_SIZE				30
#define		DURATION_SIZE				10

#define     	ARGC_NEDEED				3

#define 	TYPE_FILE				1
#define 	TYPE_DIR				2
#define 	TYPE_DOT				3

typedef struct _RECORD {

	TCHAR				ip[IP_SIZE];
	TCHAR				name[NAME_SIZE];
	TCHAR				date[DATE_SIZE];
	TCHAR				duration[DURATION_SIZE];

} RECORD;

typedef struct _THREAD_DATA {

	
	//synchronization object used to implement "first readers and writers problem"
	PHANDLE				resource;
	PHANDLE				rmutex;;
	PDWORD				readCount;

	PDWORD				numServer;
	TCHAR				**serverList;

	LPCRITICAL_SECTION csOut;


}THREAD_DATA;


//function prototype
DWORD WINAPI thWork(LPVOID param);
TCHAR  **GenerateArrayOfServer(PTCHAR fileName);
DWORD GetNumberOfServer(PTCHAR	fileName);
static DWORD FileType(LPWIN32_FIND_DATA pFileData);
DWORD WINAPI ReadDirectory(PTCHAR path, LPCRITICAL_SECTION out);
DWORD WINAPI WriteFileDirectory(PTCHAR path);
PTCHAR GenerateNewDate();
PTCHAR GenerateNewDuration();
LONG ConnectionTimeToInt(TCHAR *duration);
PTSTR IntToDur(int seconds);



INT _tmain(INT argc, LPTSTR argv[])
{
	DWORD				numThread;
	TCHAR				dirAllServer[BUFFER];
	

	PHANDLE				thHandle;
	PDWORD				thId;

	THREAD_DATA			*dataTh;

	PHANDLE				resource, rmutex;
	PDWORD				readCount;
	
	DWORD				numServer;
	TCHAR				**serverList;

	CRITICAL_SECTION    csOut; //used to print out in correct way without interleaving

	srand((unsigned)time(NULL));
	
	//checking parameters passed through the command line
	if (argc < ARGC_NEDEED) {
		fprintf(stderr, "Error: usage <filename> <numThreads>\n");
		return -1;
	}
	

	//get the paramenters
	numThread = _tstoi(argv[2]);
	_tcscpy(dirAllServer, argv[1]);

	
	//handle and id array allocation
	thHandle = (PHANDLE)malloc(numThread * sizeof(HANDLE));
	thId = (PDWORD)malloc(numThread * sizeof(DWORD));


	//structure for thread allocation
	dataTh = (THREAD_DATA*)malloc(numThread * sizeof(THREAD_DATA));


	//getting number of server from the file and array of server
	if ((numServer = GetNumberOfServer(dirAllServer)) == 0)
		return -1;
	if ((serverList = GenerateArrayOfServer(dirAllServer)) == NULL)
		return -1;

	
	resource = (PHANDLE)malloc(numServer * sizeof(HANDLE));
	rmutex = (PHANDLE)malloc(numServer * sizeof(HANDLE));
	
	//synchronization object creation;

	for (int i = 0; i < numServer; i++) {
		resource[i] = CreateSemaphore(NULL, 1, 1, NULL);
		rmutex[i] = CreateSemaphore(NULL, 1, 1, NULL);
	}

	readCount = (PDWORD)malloc(numServer * sizeof(DWORD));
	for (int i = 0; i < numServer; i++)
		readCount[i] = 0;


	InitializeCriticalSection(&csOut);

	//thread creation
	for (int i = 0; i < numThread; i++) {

		//_tcscpy(dataTh[i].dirAllServer, dirAllServer);

		dataTh[i].readCount = readCount;
		dataTh[i].resource = resource;
		dataTh[i].rmutex = rmutex;

		dataTh[i].numServer = &numServer;
		dataTh[i].serverList = serverList;

		dataTh[i].csOut = &csOut;

		thHandle[i] = CreateThread(NULL, 0, thWork, &dataTh[i], CREATE_NOT_SUSPEND, &thId[i]);

	}

	WaitForMultipleObjects(numThread, thHandle, TRUE, INFINITE);

	//close the handle of the threads
	for (int i = 0; i < numThread; i++)
		CloseHandle(thHandle[i]);

	//delete critical section
	DeleteCriticalSection(&csOut);

	system("PAUSE");
	return 0;
}


DWORD WINAPI thWork(LPVOID param) {

	THREAD_DATA			*data;
	DOUBLE				n1, n2, n3;
	INT					serverNumber;
	DWORD				id;
	
	srand(GetCurrentThreadId());
	

	data = (THREAD_DATA *)param;

	id = GetCurrentThreadId();

	//generate 3 number
	n1 = (rand() % 100) * 0.01;  //0.0 , 0.1 , 0.2 ....0.9
	n2 = (rand() % 100) * 0.01;
	n3 = (rand() % 100) * 0.01;

	serverNumber = (INT)(n3 * *data->numServer);
	
	//Sleep
	Sleep(n1 * 100);
	
	
	//cheking behavior
	if (n2 >= 0 && n2 < 0.5) {
		//	R E A D E R 

		if (WaitForSingleObject(data->rmutex[serverNumber], INFINITE) != WAIT_OBJECT_0)
			return -1;
			
		data->readCount[serverNumber]++;
		if (data->readCount[serverNumber] == 1)
			if (WaitForSingleObject(data->resource[serverNumber], INFINITE) != WAIT_OBJECT_0)
				return -2;
		
		if (ReleaseSemaphore(data->rmutex[serverNumber], 1, NULL) == 0)
			return  -3;

		
		//reading
		ReadDirectory(data->serverList[serverNumber], data->csOut);


		if (WaitForSingleObject(data->rmutex[serverNumber], INFINITE) != WAIT_OBJECT_0)
			return -4;
		data->readCount[serverNumber]--;
		if (data->readCount[serverNumber] == 0)
			if (ReleaseSemaphore(data->resource[serverNumber], 1, NULL) == 0) {
				fprintf(stderr, "error : %d", GetLastError());
				return -5;
			}
				
		if (ReleaseSemaphore(data->rmutex[serverNumber], 1, NULL) == 0)
			return -6;

	}

	//cheking behavior
	if (n2 >= 0.5 && n2 < 1) {
		//	W R T I T E R
			
		if (WaitForSingleObject(data->resource[serverNumber], INFINITE) != WAIT_OBJECT_0)
			return -7;

		//writing
		WriteFileDirectory(data->serverList[serverNumber]);

		if (ReleaseSemaphore(data->resource[serverNumber], 1, NULL) == 0) {
			fprintf(stderr, "%d error : %d\n", id , GetLastError());
			return -8;
		}

	}

	return 0;

}



DWORD GetNumberOfServer( PTCHAR	fileName) {

	HANDLE			file;
	DWORD			numServer;
	DWORD			nIn;

	file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return -2;
	}

	if (ReadFile(file, &numServer, sizeof(DWORD), &nIn, NULL) == FALSE) {
		fprintf(stderr, "Error during read from file %d", GetLastError());
		return 0;
	}

	_tprintf(_T("Number of Server %u\n"), numServer);

	CloseHandle(file);

	return numServer;
}

TCHAR  **GenerateArrayOfServer(PTCHAR fileName) {


	HANDLE			file;
	DWORD			numServer;
	DWORD			nIn;
	INT				index = 0;

	TCHAR			**serverList;

	

	file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return NULL;
	}

	//get server number
	if (ReadFile(file, &numServer, sizeof(DWORD), &nIn, NULL) == FALSE) {
		fprintf(stderr, "Error during read from file :d", GetLastError());
		return NULL;
	}


	//list of directory of each server
	serverList = (TCHAR **)malloc(numServer * sizeof(TCHAR *));
	for (int i = 0; i < numServer; i++)
		serverList[i] = (TCHAR *)malloc(BUFFER * sizeof(TCHAR));


	while ((ReadFile(file, serverList[index], BUFFER * sizeof(TCHAR), &nIn, NULL) > 0) && nIn > 0) {
		
		_tprintf(_T("Server : %s\n"), serverList[index]);
		index++;
	}


	CloseHandle(file);
	return serverList;

}


DWORD WINAPI ReadDirectory(PTCHAR path , LPCRITICAL_SECTION out ){
	HANDLE				SearchHandle;
	WIN32_FIND_DATA		FindData;
	DWORD				FType, i;
	HANDLE				file;
	DWORD				id;
	RECORD				record;
	DWORD				nIn;

	TCHAR				bufferFile[BUFFER] = { 0 };
	TCHAR				buffer[BUFFER] = { 0 };

	LONG				totConnectonTime = 0;

	TCHAR				lastAccessClock[] = { _T("0000/00/00:00:00:00") };



	//get thread id
	id = GetCurrentThreadId();

	_tcscat(buffer, path);
	_tcscat(buffer, _T("\\*"));

	SearchHandle = FindFirstFile(buffer, &FindData);

	do {
		FType = FileType(&FindData);


		if (FType == TYPE_FILE) {

			//no interleaving
			EnterCriticalSection(out);
			
			//absolute path generation seeing that with use of thread SetCurrentDirectory dosen't work
			//becuase when it comes called it changes the current directory for eache thread
			_tcscat(bufferFile, path);
			_tcscat(bufferFile, _T("\\"));
			_tcscat(bufferFile, FindData.cFileName);

			_tprintf(_T("\n\n"));

			_tprintf(_T("%d File Name to read : %s\n"), id , FindData.cFileName);

			file = CreateFile(bufferFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
				return -1;
			}

			while ((ReadFile(file, &record, sizeof(RECORD), &nIn, NULL)) > 0 && nIn > 0) {
				_tprintf(_T("%s %s %s %s\n"), record.ip, record.name, record.date, record.duration);
				totConnectonTime += ConnectionTimeToInt(record.duration);

				//checking last access 
				if (_tcscmp(lastAccessClock, record.date) < 0)
					_tcscpy(lastAccessClock, record.date);

			}
				
			
			
			CloseHandle(file);
			LeaveCriticalSection(out);

			memset(bufferFile, 0, BUFFER);
		}

	} while (FindNextFile(SearchHandle, &FindData));


	FindClose(SearchHandle);

	_tprintf(_T("Connection Time for %s Server : %s  Last Access : %s\n"), path , IntToDur(totConnectonTime) , lastAccessClock);

	return 0;
}



DWORD WINAPI WriteFileDirectory(PTCHAR path) {
	HANDLE				SearchHandle;
	WIN32_FIND_DATA		FindData;
	DWORD				FType, i;
	HANDLE				file;
	DWORD				id;
	RECORD				record;
	DWORD				nIn, nOut;

	TCHAR				bufferFile[BUFFER] = { 0 };
	TCHAR				buffer[BUFFER] = { 0 };

	DWORD				choise = 0;

	OVERLAPPED			overlapped = { 0, 0, 0, 0, NULL };
	LARGE_INTEGER		filePos;
	LARGE_INTEGER		fileReserved;

	DWORD				index = 0;

	

	//get thread id
	id = GetCurrentThreadId();

	_tcscat(buffer, path);
	_tcscat(buffer, _T("\\*"));

	SearchHandle = FindFirstFile(buffer, &FindData);

	do {
		FType = FileType(&FindData);

		if (FType == TYPE_FILE) {

		
			//absolute path generation seeing that with use of thread SetCurrentDirectory dosen't work
			//becuase when it comes called it changes the current directory for eache thread
			_tcscat(bufferFile, path);
			_tcscat(bufferFile, _T("\\"));
			_tcscat(bufferFile, FindData.cFileName);

			_tprintf(_T("\n\n"));

			_tprintf(_T("%d File Name to Modify : %s\n"), id, FindData.cFileName);

			file = CreateFile(bufferFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
				return -1;
			}

			

			index = 0;

			//loking generation;
			filePos.QuadPart = index * sizeof(RECORD);  //set the position on the file
			fileReserved.QuadPart = sizeof(RECORD);				// block only one row

																//overlapped strcutre generation
			overlapped.Offset = filePos.LowPart;
			overlapped.OffsetHigh = filePos.HighPart;
			overlapped.hEvent = (HANDLE)0;


			while (ReadFile(file, &record, sizeof(RECORD), &nIn, &overlapped) > 0 && nIn > 0 ) {

				
				//decision about what modify
				choise = (rand() % 2);
				if (choise == 0) //modify acced date and time
					_tcscpy(record.date, GenerateNewDate());

				else if (choise == 1)  //modify accedd duration
					_tcscpy(record.duration, GenerateNewDuration());

				
				//write the updated ecord on the file in the same position in which has been read
				if (WriteFile(file, &record, sizeof(RECORD), &nOut, &overlapped) == FALSE) {
					fprintf(stderr, "Error %x", GetLastError());
					return -10;
				}

			
				_tprintf(_T("%s %s %s %s\n"), record.ip, record.name, record.date, record.duration);

				//increment the index used for read with an overlapped structure
				index++;

				//loking generation;
				filePos.QuadPart = index * sizeof(RECORD);  //set the position on the file
				fileReserved.QuadPart = sizeof(RECORD);				// block only one row

																	//overlapped strcutre generation
				overlapped.Offset = filePos.LowPart;
				overlapped.OffsetHigh = filePos.HighPart;
				overlapped.hEvent = (HANDLE)0;
				
			}
				

			CloseHandle(file);
			memset(bufferFile, 0, BUFFER);
		}

	} while (FindNextFile(SearchHandle, &FindData));


	FindClose(SearchHandle);

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



PTCHAR GenerateNewDate() {

	PTCHAR		randomList[] = { L"2017/03/06:13:55:36" , L"2018/08/16:16:55:36" , L"2017/01/01:13:11:44" };
	DWORD		randomListSize = 3;

	
	return randomList[(rand() % randomListSize)];
}

PTCHAR GenerateNewDuration() {

	PTCHAR		randomList[] = { L"00:55:36" , L"04:03:44" , L"03:11:44" };
	DWORD		randomListSize = 3;


	return randomList[(rand() % randomListSize)];
}

LONG ConnectionTimeToInt(TCHAR *duration) {

	DWORD			seconds, minutes, hours;
	TCHAR			cseconds[2], cminutes[2], chours[2];

	hours = minutes = seconds = 0;

	cseconds[0] = duration[6];
	cseconds[1] = duration[7];

	cminutes[0] = duration[3];
	cminutes[1] = duration[4];

	chours[0] = duration[0];
	chours[1] = duration[1];

	seconds = _tstoi(cseconds);
	minutes = _tstoi(cminutes);
	hours = _tstoi(chours);

	return (hours * 60 * 60) + (minutes * 60) + seconds;

	
}




PTSTR IntToDur(int seconds) {

	LPTSTR result = (LPTSTR)calloc(9, sizeof(TCHAR));


	int h1 = seconds / (60 * 60 * 10);
	int h2 = (seconds - (h1 * 60 * 60 * 10)) / (60 * 60);
	int m1 = (seconds - (h1 * 60 * 60 * 10) - (h2 * 60 * 60)) / (60 * 10);
	int m2 = (seconds - (h1 * 60 * 60 * 10) - (h2 * 60 * 60) - (m1 * 60 * 10)) / 60;
	int s1 = (seconds - (h1 * 60 * 60 * 10) - (h2 * 60 * 60) - (m1 * 60 * 10) - (m2 * 60)) / 10;
	int s2 = seconds - (h1 * 60 * 60 * 10) - (h2 * 60 * 60) - (m1 * 60 * 10) - (m2 * 60) - (s1 * 10);

	result[0] = h1 + _T('0');
	result[1] = h2 + _T('0');
	result[2] = _T(':');
	result[3] = m1 + _T('0');
	result[4] = m2 + _T('0');
	result[5] = _T(':');
	result[6] = s1 + _T('0');
	result[7] = s2 + _T('0');
	result[8] = _T('\0');


	return result;

}


