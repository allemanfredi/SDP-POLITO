



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <string.h>

typedef struct _threadData{
	
	int		id;
	int 		k;
	int     	time;
	sem_t		*na;
	sem_t   	*cl;
	sem_t		*print_na;
	sem_t		*print_cl;
	
	
	

}threadData;




pthread_t		t1 , t2 ;
threadData		data1[2] ;

sem_t	*na , *cl , *printna , *printcl;

int	c_na , c_cl;
int     print_na , print_cl;




void *atomsOfSodium ( void *param );
void *atomOfSodium ( void *param );
void *atomsOfClhorine ( void *param );
void *atomOfClhorine ( void *param );
void *printt ( void *param );


int main ( int argc , char *argv[] ){
	
	srand(time(NULL));

	int 		k = 6;
	k = atoi(argv[1]);

	//semaphore allocation
	na 	= ( sem_t * ) malloc ( sizeof ( sem_t ) );	
	cl 	= ( sem_t * ) malloc ( sizeof ( sem_t ) );	
	printna   = ( sem_t * ) malloc ( sizeof ( sem_t ) );
	printcl   = ( sem_t * ) malloc ( sizeof ( sem_t ) );	

	//mutexes allocation
	mna 	= ( pthread_mutex_t * ) malloc ( sizeof ( pthread_mutex_t ) );	
	mcl 	= ( pthread_mutex_t * ) malloc ( sizeof ( pthread_mutex_t ) );	
	
	
	print_na = print_cl = 0;

	
	sem_init( na , 0 , 1 );
	sem_init( cl , 0 , 1 );
	sem_init( printna , 0 , 0 );
	sem_init( printcl , 0 , 0 );


	data1[0].id   	= 0;
	data1[0].k    	= k;
	data1[0].time 	= rand() %5; //0-4 seconds
	data1[0].na   	= na;
	data1[0].cl   	= cl;
	data1[0].mna    = mna;
	data1[0].mcl    = mcl;
	data1[0].print_cl    = printcl;
	data1[0].print_na  = printna;


	data1[1].id 	= 1;
	data1[1].k  	= k;
	data1[1].time 	= rand() %5; //0-4 seconds
	data1[1].na   	= na;
	data1[1].cl   	= cl;
	data1[1].mna    = mna;
	data1[1].mcl    = mcl;
	data1[0].print_cl    = printcl;
	data1[0].print_na  = printna;




	pthread_create ( &t1 , NULL , atomsOfSodium ,   &data1[0] );
	pthread_create ( &t2 , NULL , atomsOfClhorine , &data1[1] );

	pthread_join( t1 , NULL );
	
	

	return 0;
}






void *atomOfSodium ( void * param ) {
	
	threadData 	*data = ( threadData * ) param;


	sem_wait ( data->na );

	c_na = data->id;

	sem_post ( data->print_cl );
	sem_wait ( data->print_na );

	fprintf ( stdout , "id %d na %d cl %d \n" , data->id , c_na , c_cl );

	sem_post ( data->na );
	
	
	return NULL;
	
	

}

void *atomOfClhorine ( void * param ) {

	
	threadData 	*data = ( threadData * ) param;


	sem_wait ( data->cl );

	c_cl = data->id;

	sem_post ( data->print_na );
	sem_wait ( data->print_cl );

	fprintf ( stdout , "id %d na %d cl %d \n" , data->id , c_na , c_cl );

	sem_post ( data->cl );
	
	
	return NULL;
	
	

}



void *atomsOfSodium ( void *param ){

	pthread_t	*thSodium;
	int		i;

	threadData *data = ( threadData *) param;

	threadData *atomsData = ( threadData *) malloc ( data->k * sizeof ( threadData ) );
	
	thSodium = ( pthread_t * ) malloc ( data->k * sizeof ( threadData ) );

	

	for ( i = 0; i < data->k; i++ ){
 
		atomsData[i].k   	= data->k;
		atomsData[i].id   	= i;
		atomsData[i].time 	= 0; //not needed
		atomsData[i].na   	= data->na;
		atomsData[i].cl   	= data->cl;

		atomsData[i].mna    	= data->mna;
		atomsData[i].mcl    	= data->mcl;
		atomsData[i].print_na  = printna;
		atomsData[i].print_cl  = printcl;
		
		
		pthread_create ( &thSodium[i] , NULL , atomOfSodium ,   &atomsData[i] );

		sleep(data->time);
	}

}






void *atomsOfClhorine ( void *param ){

	pthread_t	*thClhorine;
	int		i;

	threadData *data = ( threadData *) param;

	threadData *atomsData = ( threadData *) malloc ( data->k * sizeof ( threadData ) );
	
	thClhorine = ( pthread_t * ) malloc ( data->k * sizeof ( threadData ) );

	

	for ( i = 0; i < data->k; i++ ){
 
		atomsData[i].k   	= data->k;
		atomsData[i].id   	= i + data->k;
		atomsData[i].time 	= 0; //not needed
		atomsData[i].na   	= data->na;
		atomsData[i].cl   	= data->cl;
		atomsData[i].mna    	= data->mna;
		atomsData[i].mcl    	= data->mcl;
		atomsData[i].print_na  = printna;
		atomsData[i].print_cl  = printcl;
		
		
		pthread_create ( &thClhorine[i] , NULL , atomOfClhorine ,   &atomsData[i] );

		sleep(data->time);
	}

	
	
}


