#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <string.h>

typedef struct _threadDataA{
	
	int			id;
	int 			n;
	
	sem_t			*sa;
	sem_t			*sb;
	

}threadDataA;

typedef struct _threadDataB{
	
	int			id;
	int 			n;

	sem_t			*sa;
	sem_t			*sb;
	

}threadDataB;



//variable declaration
threadDataA *dataThA;
threadDataB *dataThB;

pthread_t  *thA , *thB;
sem_t	sa , sb ;

int cnt_a , cnt_b;
int id_a[2] , id_b[2];


//function prototype
void *thAproc ( void *param );
void *thBproc ( void *param );


int main ( int argc , char *argv[] ){
	
	srand(time(NULL));

	int n = atoi ( argv[1] );


	//thread allocation
	thA = ( pthread_t * ) malloc ( n * sizeof ( pthread_t ) );
	thB = ( pthread_t * ) malloc ( n * sizeof ( pthread_t ) );

	//structure allocation
	dataThA = ( threadDataA * ) malloc ( n * sizeof ( threadDataA ) );
	dataThB = ( threadDataB * ) malloc ( n * sizeof ( threadDataB ) );

	

	sem_init ( &sa , 0 , 2 );
	sem_init ( &sb , 0 , 2 );

	
	for ( int i = 0; i < n; i++ ){
		//A
		dataThA[i].n    = n;
		dataThA[i].id 	= i;
		dataThA[i].sa	= &sa;
		dataThA[i].sb	= &sb;


		//B
		dataThB[i].n    = n;
		dataThB[i].id 	= i;
		dataThB[i].sb	= &sb;
		dataThB[i].sa	= &sa;
		
		pthread_create ( &thA[i] , NULL , thAproc , &dataThA[i] );
		pthread_create ( &thB[i] , NULL , thBproc , &dataThB[i] );
	}
	
	for ( int i = 0; i < n; i++ ){
		pthread_join ( thA[i] , NULL );
		pthread_join ( thB[i] , NULL );

	}
	

	return 0;
}




void *thAproc ( void *param ){

	threadDataA *data = ( threadDataA * ) param;

	int s = rand() % 4; //[0,3]
	sleep(s);

	sem_wait ( data->sa );

	id_a[cnt_a] = data->id;

	cnt_a++;
	if ( cnt_a == 2 ){
	
		
		fprintf ( stdout , "A%d cats A%d A%d\n" , data->id , id_a[1] , id_a[0] );
		
		if ( ( cnt_a + cnt_b ) == 4 ){

			fprintf ( stdout , "	A%d merge  A%d A%d B%d B%d\n" , data->id , id_a[1] , id_a[0] ,id_b[1] , id_b[0] );

			cnt_a = 0;
			cnt_b = 0;
			sem_post(data->sa);
			sem_post(data->sa);
			sem_post(data->sb);
			sem_post(data->sb);
			
		}
			
	}


}


void *thBproc ( void *param ){

	threadDataB *data = ( threadDataB * ) param;

	int s = rand() % 4; //[0,3]
	sleep(s);

	sem_wait ( data->sb );
	
	id_b[cnt_b] = data->id;

	
	cnt_b++;
	if ( cnt_b == 2 ){
	

		fprintf ( stdout , "B%d cats B%d B%d\n" , data->id , id_b[1] , id_b[0] );

		if ( ( cnt_a + cnt_b ) == 4 ){

			fprintf ( stdout , "	B%d merge  B%d B%d A%d A%d\n" , data->id , id_b[1] , id_b[0] ,id_a[1] , id_a[0] );
			cnt_a = 0;
			cnt_b = 0;			
			sem_post(data->sa);
			sem_post(data->sa);
			sem_post(data->sb);
			sem_post(data->sb);
			
		}
		
	
	}


}


