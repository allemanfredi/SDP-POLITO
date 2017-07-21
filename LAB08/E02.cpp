// ConsoleApplication3.cpp : definisce il punto di ingresso dell'applicazione console.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

#define BUF_SIZE				100
#define NUMBER_OF_PARAMETERS	2


#define ERROR_FILE_OPENING		-2
#define STUDENT_NOT_FOUND		-1


struct student {
	DWORD id;
	DWORD registerNumber;
	CHAR name[30];
	CHAR surname[30];
	DWORD vote;

};


LPTSTR progName;
LPTSTR fileName;


// function prototype
BOOL SearchStudentById(LPTSTR fileName, INT id);
BOOL OverwriteStudentById(LPTSTR fileName, INT id);
void PrintStudentData(student *s);


INT _tmain(INT argc, LPTSTR argv[])
{
	CHAR		ch;
	CHAR		cmd[BUF_SIZE];
	INT			i = 0;
	BOOL		error;
	INT			id;


	memset(cmd, 0, BUF_SIZE);
	progName = argv[0];

	if (argc != NUMBER_OF_PARAMETERS)
	{
		fprintf(stdout, "(%s) -----> Error: usage <file_name>\n");
		return -1;

	}

	fileName = argv[1];


	for (;;)
	{
		fprintf(stdout, "(%s) <R> <N = student Id>\n", progName);
		fprintf(stdout, "(%s) <W> <N = student Id>\n", progName);
		fprintf(stdout, "(%s) <E> Exit\n", progName);


		//getting command
		while ((ch = fgetchar()) != '\n') {
			cmd[i] = ch;
			i++;
		}
		i = 0;


		// get the integer number from command  jumping the first and the second character ( ex "R" and space )
		id = atoi(cmd + 2);

		switch (cmd[0]) {

		case 'R':
		{

			error = SearchStudentById(fileName, id);
			if (error == ERROR_FILE_OPENING)
				fprintf(stderr, "(%s) Cannot open input file. Error: %x\n", GetLastError());
			if (error == STUDENT_NOT_FOUND)
				fprintf(stderr, "(%s) Student not found\n", progName);

			break;
		}


		case 'W':
		{
			error = OverwriteStudentById(fileName, id);
			if (error == ERROR_FILE_OPENING)
				fprintf(stderr, "(%s) Cannot open input file. Error: %x\n", GetLastError());
			if (error == STUDENT_NOT_FOUND)
				fprintf(stderr, "(%s) Student not found\n", progName);

			break;
		}

		case 'E':
		{

			exit(0);
			break;
		}
		default:
			fprintf(stdout, "(%s) -----> Error: invalid input command\n", progName);



		}
		memset(cmd, 0, BUF_SIZE);
		ch = 0;
		system("pause");
		system("cls");



	}



	return 0;
}



BOOL SearchStudentById(LPTSTR fileName, INT id)
{
	HANDLE				hIn;
	student				s;
	DWORD				nIn;
	OVERLAPPED			ov = { 0, 0, 0, 0, NULL };
	LARGE_INTEGER		FilePos;
	INT					n = 0;


	//file opening
	hIn = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE)
		return -2;



	//reading from file
	while (ReadFile(hIn, &s, sizeof(student), &nIn, &ov) > 0 && nIn > 0)
	{
		FilePos.QuadPart = n * sizeof(student);
		ov.Offset = FilePos.LowPart;
		ov.OffsetHigh = FilePos.HighPart;

		if (s.id == id)
		{
			PrintStudentData(&s);
			CloseHandle(hIn);
			return 1;
		}
		n++;
	}

		


	// student not found
	CloseHandle(hIn);
	return -1;
}


void PrintStudentData(student *s)
{
	fprintf(stdout, "name : %s\n", s->name);
	fprintf(stdout, "surname : %s\n", s->surname);
	fprintf(stdout, "registration Number : %d\n", s->registerNumber);
	fprintf(stdout, "id : %d\n", s->id);
	fprintf(stdout, "vote : %d\n\n", s->vote);

}


BOOL OverwriteStudentById(LPTSTR fileName, INT id)
{
	HANDLE				hIn;
	student				s;
	DWORD				nIn;
	DWORD				nOut;
	OVERLAPPED			ov = { 0, 0, 0, 0, NULL };
	LARGE_INTEGER		FilePos;
	INT					n = 0;


	//file opening
	hIn = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE)
		return -2;


	
	//file reading
	while (ReadFile(hIn, &s, sizeof(student), &nIn, &ov) > 0 && nIn > 0) 
	{

		if (s.id == id)
		{
			s.registerNumber = 333333333; // i do in this way because i didn't understant what the prof wants because he say:"the user want to add data for student x"
			
			FilePos.QuadPart = n * sizeof(student);
			
			ov.Offset = FilePos.LowPart;
			ov.OffsetHigh = FilePos.HighPart;

			WriteFile(hIn, &s, sizeof(student), &nOut, &ov);

			CloseHandle(hIn);
			return 1;
		}
		n++;

		FilePos.QuadPart = n * sizeof(student);
		ov.Offset = FilePos.LowPart;
		ov.OffsetHigh = FilePos.HighPart;
	}
		


	// student not found
	CloseHandle(hIn);
	return -1;






}
