
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <time.h>


#ifndef _UNICODE
#define UNICODE
#endif // !_UNICODE


#define		  BUFFER					500
#define		  CREATE_NOT_SUSPEND		0
#define     START_LIST_SIZE			5


typedef struct _record {

	TCHAR id[20];
	TCHAR name[20];
	TCHAR surname[20];
	TCHAR sex;
	TCHAR arrivalTime[6];
	DWORD timeToRegister;
	DWORD timeRequested;

} RECORD;


typedef struct _LIST {
	RECORD	    *buffer;  //list of all record
	INT			 size;
	INT			 read;
	INT			 write;

}LIST;


typedef struct _DATA_TO_START {

	LPTSTR		filename;
	LIST		  *startQueue;


}DATA_TO_START;

typedef struct _DATA_FOR_CHECK_SEX {

	LIST				        *votersQueue;
	
	PHANDLE				      fullMale;
	PHANDLE				      emptyMale;
	PCRITICAL_SECTION	  csMale;

	PHANDLE				      fullFemale;
	PHANDLE				      emptyFemale;
	PCRITICAL_SECTION	  csFemale;

	RECORD				      *maleStation;
	RECORD				      *femaleStation;


}DATA_FOR_CHECK_SEX;


typedef struct _DATA_FOR_MALE {

	LIST				        *votersQueue;
	PHANDLE				      fullMale;
	PHANDLE				       emptyMale;
	PCRITICAL_SECTION	  csMale;
	RECORD				      *maleStation;
	LIST				        *votersQueueM;
	PHANDLE				      fullQueueM;
	PHANDLE				      emptyQueueM;
	LPCRITICAL_SECTION  csQueueM;
	PDWORD			        hashFreeStation;
	PCRITICAL_SECTION	  csHashFreeStation;
	DWORD				        n;

}DATA_FOR_MALE;

typedef struct _DATA_FOR_FEMALE {

	LIST				        *votersQueue;
	PHANDLE				      fullFemale;
	PHANDLE				      emptyFemale;
	PCRITICAL_SECTION   csFemale;
	RECORD				      *femaleStation;  //used to store the voting person
	LIST				        *votersQueueM;
	PHANDLE				      fullQueueM;
	PHANDLE				      emptyQueueM;
	LPCRITICAL_SECTION  csQueueM;
	PDWORD			        hashFreeStation;
	PCRITICAL_SECTION	  csHashFreeStation;
	DWORD				        n;

}DATA_FOR_FEMALE;


typedef struct  _DATA_FOR_STATION {


	LIST				          *votersQueueM;
	PHANDLE				        fullQueueM;
	PHANDLE				        emptyQueueM;
	LPCRITICAL_SECTION    csQueueM;
	PDWORD			          hashFreeStation;
	DWORD				          n;
	DWORD				          indexInhashFreeStation;
	PCRITICAL_SECTION	    csHashFreeStation;


}DATA_FOR_STATION;


BOOL isDone = false;
BOOL isDoneMaleAndFemale = false;


//function protoype for queue
VOID init(LIST* ptr, INT size);
DWORD insert(LIST * ptr, RECORD value);
VOID visit(LIST * ptr);
RECORD *remove(LIST * ptr);


DWORD  readFromFile(VOID *param);

//THREADs
DWORD WINAPI male(VOID *param);
DWORD WINAPI female(VOID *param);
DWORD WINAPI station(VOID *param);
DWORD WINAPI checkSex(VOID *param);



INT _tmain(INT argc, LPTSTR argv[])
{
	DWORD						      n, m;
	LPTSTR						    iFile, oFile;
	HANDLE						    hMale ,hFemale, hCheckSex;;
	PHANDLE						    hStation;
  DWORD						      idMale, idFemale , idCheckSex;
	PDWORD						    idStation;

	HANDLE						    fullMale , fullFemale;
	HANDLE						    emptyMale, emptyFemale;
	CRITICAL_SECTION			csMale , csFemale;

	LIST						      votersQueue1, votersQueue2;


	DATA_FOR_CHECK_SEX		dataCheck;
	DATA_FOR_MALE				  dataMale;
	DATA_FOR_FEMALE				dataFemale;
	DATA_TO_START				  dataStart;
	DATA_FOR_STATION			*dataStation;

	RECORD						    maleStation;
	RECORD					  	  femaleStation;

	PHANDLE						    fullQueueM;
	PHANDLE						    emptyQueueM;
	PCRITICAL_SECTION			csQueueM;

	PDWORD						    hashFreeStation; //used to keep in mind which stattion are avaiable and which not

	CRITICAL_SECTION			csHashFreeStation;


	
	//getting parameter from command line
	n		= _tstoi(argv[2]);
	m		= _tstoi(argv[3]);
	iFile	= argv[1];
	oFile	= argv[4];


	//array of handle to managed N-station-thread
	hStation  = (PHANDLE)malloc(n * sizeof(HANDLE));
	idStation = (PDWORD)malloc(n * sizeof(HANDLE));
	dataStation = (DATA_FOR_STATION*)malloc(n * sizeof(DATA_FOR_STATION));

	fullQueueM = (PHANDLE)malloc(n * sizeof(HANDLE));
	emptyQueueM = (PHANDLE)malloc(n * sizeof(HANDLE));
	csQueueM = (PCRITICAL_SECTION)malloc(n * sizeof(CRITICAL_SECTION));
	hashFreeStation = (PDWORD)malloc(n * sizeof(DWORD));
	for (int i = 0; i < n; i++)
		hashFreeStation[i] = 0;
	


	//queue allocation
	init(&votersQueue1, START_LIST_SIZE + 1);
	init(&votersQueue2, m);
	//init(&startQueue, m);



	
	//syncronizatihion obect allocation for male and female and 
	fullMale = CreateSemaphore(NULL, 0, 1, NULL);
	emptyMale = CreateSemaphore(NULL, 1,1, NULL);
	InitializeCriticalSection(&csMale);

	fullFemale = CreateSemaphore(NULL, 0, 1, NULL);
	emptyFemale = CreateSemaphore(NULL, 1, 1, NULL);
	InitializeCriticalSection(&csFemale);

	InitializeCriticalSection(&csHashFreeStation);

	

	//semaphore for last queue initialization
	for (int i = 0; i < n; i++) {
		fullQueueM[i] = CreateSemaphore(NULL, 0, m, NULL);
		emptyQueueM[i] = CreateSemaphore(NULL, m, m, NULL);
		InitializeCriticalSection(&csQueueM[i]);
	}





	//struct allocation for open file and fill the voter's queue 1 and thread allocation
	dataStart.filename = iFile;
	dataStart.startQueue = &votersQueue1;

	//fill the first queue and after that i can start with the sycncronization;
	readFromFile(&dataStart);



	//struct  for check sex initializaiton and thread creation
	dataCheck.csMale = &csMale;
	dataCheck.emptyMale = &emptyMale;
	dataCheck.fullMale = &fullMale;
	dataCheck.csFemale = &csFemale;
	dataCheck.emptyFemale = &emptyFemale;
	dataCheck.fullFemale = &fullFemale;
	dataCheck.votersQueue = &votersQueue1;
	dataCheck.maleStation = &maleStation;
	dataCheck.femaleStation = &femaleStation;
	
	//thread to check the sex creation
	hCheckSex = CreateThread(NULL, 0, checkSex, &dataCheck, CREATE_NOT_SUSPEND, &idCheckSex);

	
	
	//struct and thread for separating the male and female in order to vote and thread creation
	//male
	dataMale.csMale = &csMale;
	dataMale.emptyMale = &emptyMale;
	dataMale.fullMale = &fullMale;
	dataMale.maleStation = &maleStation;
	dataMale.votersQueue = &votersQueue1;
	dataMale.csQueueM = csQueueM;
	dataMale.fullQueueM = fullQueueM;
	dataMale.emptyQueueM = emptyQueueM;
	dataMale.votersQueueM = &votersQueue2;
	dataMale.hashFreeStation = hashFreeStation;
	dataMale.n = n;
	dataMale.csHashFreeStation = &csHashFreeStation;
	

	//female
	dataFemale.csFemale = &csFemale;
	dataFemale.emptyFemale = &emptyFemale;
	dataFemale.fullFemale = &fullFemale;
	dataFemale.femaleStation = &femaleStation;
	dataFemale.votersQueue = &votersQueue1;
	dataFemale.csQueueM = csQueueM;
	dataFemale.fullQueueM = fullQueueM;
	dataFemale.emptyQueueM = emptyQueueM;
	dataFemale.votersQueueM = &votersQueue2;
	dataFemale.hashFreeStation = hashFreeStation;
	dataFemale.n = n;
	dataFemale.csHashFreeStation = &csHashFreeStation;
	



	//thread for voting both male and female
	hMale = CreateThread(NULL, 0,   male,   &dataMale,   CREATE_NOT_SUSPEND, &idMale);
	hFemale = CreateThread(NULL, 0, female, &dataFemale, CREATE_NOT_SUSPEND, &idFemale);


	for (INT i = 0; i < n; i++) {

		dataStation[i].csQueueM = &csQueueM[i];
		dataStation[i].emptyQueueM = &emptyQueueM[i];
		dataStation[i].fullQueueM = &fullQueueM[i];
		dataStation[i].votersQueueM = &votersQueue2;
		dataStation[i].hashFreeStation = hashFreeStation;
		dataStation[i].n = n;
		dataStation[i].indexInhashFreeStation = i;
		dataStation[i].csHashFreeStation = &csHashFreeStation;
		

		hStation[i] = CreateThread(NULL, 0, station, &dataStation[i], CREATE_NOT_SUSPEND, &idStation[i]);
	}




	WaitForSingleObject(hCheckSex, INFINITE);

	isDone = true;
	ReleaseSemaphore(fullMale, 1, NULL);
	ReleaseSemaphore(fullFemale, 1, NULL);

	WaitForSingleObject(hMale, INFINITE);
	WaitForSingleObject(hFemale, INFINITE);

	isDoneMaleAndFemale = true;
	//wake up all process blocked in order to stop them
	for (int i = 0; i < n; i++)
		ReleaseSemaphore(fullQueueM[i], 1, NULL);


	WaitForMultipleObjects(n, hStation, TRUE, INFINITE);

	system("pause");


	return 0;
}


DWORD readFromFile(VOID *param) {

	HANDLE				iFile;
	RECORD				record;
	DWORD				nIn;
	DATA_TO_START		*data;

	data = (DATA_TO_START *)param;

	iFile = CreateFile(data->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (iFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Cannot open input file. Error: %x\n", GetLastError());
		return -2;
	}

	//start
	while (ReadFile(iFile, &record, sizeof(RECORD), &nIn, NULL) > 0 && nIn > 0) {
		//_tprintf(_T("%s %s %s\n"), record.id, record.name, record.surname);
		insert(data->startQueue, record);
	}
		


	CloseHandle(iFile);

	return 0;
	
}


DWORD WINAPI checkSex(VOID *param) {


	DATA_FOR_CHECK_SEX				*data;
	RECORD							      *record = NULL;

	data = (DATA_FOR_CHECK_SEX *)param;


	while ((record = remove(data->votersQueue)) != NULL ) {

		if (record->sex == 'M') {

			if (WaitForSingleObject(*data->emptyMale, INFINITE) != WAIT_OBJECT_0) {
				fprintf(stderr, "Error 1: %d\n", GetLastError());
				return -2;
			}
			
			EnterCriticalSection(data->csMale);
			//put in the correspondignrestratiion station
			*data->maleStation = *record;
			LeaveCriticalSection(data->csMale);

			if (ReleaseSemaphore(*data->fullMale, 1, NULL) == 0) {
				fprintf(stderr, "Error 2: %d\n", GetLastError());
				return -3;
			}
		}
		if (record->sex == 'F') {


			if (WaitForSingleObject(*data->emptyFemale, INFINITE) != WAIT_OBJECT_0) {
				fprintf(stderr, "Error 3: %d\n", GetLastError());
				return -4;
			}
			EnterCriticalSection(data->csFemale);
			*data->femaleStation = *record;
			LeaveCriticalSection(data->csFemale);

			if (ReleaseSemaphore(*data->fullFemale, 1, NULL) == 0) {
				fprintf(stderr, "Error 4: %d\n", GetLastError());
				return -5;
			}
		}		
	}
  return 0;
}



DWORD WINAPI male(VOID *param) {

	DATA_FOR_MALE					*data;
	RECORD							*record = NULL;
	INT								i;

	data = (DATA_FOR_MALE *)param;


	while (true) {


		if (WaitForSingleObject(*data->fullMale, INFINITE) != WAIT_OBJECT_0) {
			fprintf(stderr, "Error 5: %d\n", GetLastError());
			return -6;

		}
		if (isDone) return 0;
		EnterCriticalSection(data->csMale);
		


		_tprintf(_T("%s is registering\n"), data->maleStation->id);
		//now i have to wait time to register
		Sleep(data->maleStation->timeToRegister);
		_tprintf(_T("%s is registered\n"), data->maleStation->id);
		//here i have to simulate a prodcuer for the station

		
			EnterCriticalSection(data->csHashFreeStation);
			for (i = 0; i < data->n; i++)
				if (data->hashFreeStation[i] == 0)
					break;
			data->hashFreeStation[i] = 1;
			LeaveCriticalSection(data->csHashFreeStation);


			if (WaitForSingleObject(data->emptyQueueM[i], INFINITE) != WAIT_OBJECT_0) {
				fprintf(stderr, "Error 10: %d\n", GetLastError());
				return -10;
			}

			EnterCriticalSection(&data->csQueueM[i]);
			//put in the corresponding registration station
			
			
			insert(data->votersQueueM, *data->maleStation);
			LeaveCriticalSection(&data->csQueueM[i]);

			if (ReleaseSemaphore(data->fullQueueM[i], 1, NULL) == 0) {
				fprintf(stderr, "Error 11: %d\n", GetLastError());
				return -11;
			}




		LeaveCriticalSection(data->csMale);
		if (ReleaseSemaphore(*data->emptyMale, 1, NULL) == 0) {
			fprintf(stderr, "Error 7: %d\n", GetLastError());
			return -7;
		}
	}

  return 0;
}



DWORD WINAPI female(VOID *param) {

	DATA_FOR_FEMALE					*data;
	RECORD							*record;
	INT								i;

	data = (DATA_FOR_FEMALE *)param;

	while (true) {

		if (WaitForSingleObject(*data->fullFemale, INFINITE) != WAIT_OBJECT_0) {
			fprintf(stderr, "Error 8: %d\n", GetLastError());
			return -6;

		}
		if (isDone) return 0;

		EnterCriticalSection(data->csFemale);


		data->femaleStation;
		_tprintf(_T("%s is registering\n"), data->femaleStation->id);
		//now i have to wait time to register
		Sleep(data->femaleStation->timeToRegister);
		_tprintf(_T("%s is registered\n"), data->femaleStation->id);
		//here i have to simulate a prodcuer for the station


			//get the station number in order 
			EnterCriticalSection(data->csHashFreeStation);
			for (i = 0; i < data->n; i++)
				if (data->hashFreeStation[i] == 0)
					break;
			data->hashFreeStation[i] = 1;
			LeaveCriticalSection(data->csHashFreeStation);


			fprintf(stderr, "index female %d\n", i);
			if (WaitForSingleObject(data->emptyQueueM[i], INFINITE) != WAIT_OBJECT_0) {
				fprintf(stderr, "Error 10: %d\n", GetLastError());
				return -10;
			}

			EnterCriticalSection(&data->csQueueM[i]);
			//put in the correspondignrestratiion station

			
			insert(data->votersQueueM, *data->femaleStation);
			LeaveCriticalSection(&data->csQueueM[i]);

			if (ReleaseSemaphore(data->fullQueueM[i], 1, NULL) == 0) {
				fprintf(stderr, "Error 11: %d\n", GetLastError());
				return -11;
			}




		LeaveCriticalSection(data->csFemale);
		if (ReleaseSemaphore(*data->emptyFemale, 1, NULL) == 0) {
			fprintf(stderr, "Error 9: %d", GetLastError());
			return -7;
		}

	}
}






DWORD WINAPI station(VOID *param) {

	DATA_FOR_STATION				*data;
	RECORD							    *app;


	data = (DATA_FOR_STATION *)param;


	while (true) {


		if (WaitForSingleObject(*data->fullQueueM, INFINITE) != WAIT_OBJECT_0) {
			fprintf(stderr, "Error 14: %d\n", GetLastError());
			return -14;

		}
		if (isDoneMaleAndFemale) return 0;
		EnterCriticalSection(data->csQueueM);

		app = remove(data->votersQueueM);
		_tprintf(_T("%s is voting\n"), app->id);
		Sleep(app->timeRequested);

		//release this station
		EnterCriticalSection(data->csHashFreeStation);
		data->hashFreeStation[data->indexInhashFreeStation] = 0;
		LeaveCriticalSection(data->csHashFreeStation);
		_tprintf(_T("%s has voted\n"), app->id);


		LeaveCriticalSection(data->csQueueM);
		if (ReleaseSemaphore(*data->emptyQueueM, 1, NULL) == 0) {
			fprintf(stderr, "Error 15: %d\n", GetLastError());
			return -15;
		}

		


	}


}




// queue
VOID init(LIST* ptr, INT size)
{

	ptr->buffer = (RECORD *)malloc(size * sizeof(RECORD));

	ptr->size = size;
	ptr->read = 0;
	ptr->write = 0;


}


RECORD *remove(LIST * ptr)
{
	RECORD			*val;


	if (ptr->read == ptr->write) return NULL;


	val = &ptr->buffer[ptr->read];
	ptr->read = (ptr->read + 1) % ptr->size;

	return val;
}




DWORD insert(LIST * ptr, RECORD value)
{

	ptr->buffer[ptr->write] = value;
	ptr->write = (ptr->write + 1) % ptr->size;

	return 0;
}

VOID visit(LIST * ptr)
{
	int pos; //indice posizione 
	for (pos = 0; pos < ptr->write; pos = pos++)
		_tprintf(_T("%s "), ptr->buffer[pos].id);

	printf("\n");
}



