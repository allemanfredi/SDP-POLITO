#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#define	BUFFER	500


typedef struct  _LIST_SEM{

	sem_t			*sem;
	int			priority;
	struct _LIST_SEM	*next;


}LIST_SEM;


typedef struct	_PRIORITY_SEMAPHORE{

	int		cnt;
	LIST_SEM	*sem_list;
	sem_t		*mutex;


}PRIORITY_SEMAPHORE;


typedef struct _THREAD_DATA{

	int 			id;
	int			class;

	PRIORITY_SEMAPHORE	*p_sem;

	int			*ppipe;
	


}THREAD_DATA;




//function prootype
void *th_work ( void *param );

PRIORITY_SEMAPHORE * ps_init ( int value );
int ps_wait (PRIORITY_SEMAPHORE * s, int prio);
void ps_signal (PRIORITY_SEMAPHORE * s);
LIST_SEM *dequeue_sorted (LIST_SEM * head);
LIST_SEM *enqueue_sorted (LIST_SEM * head , int prio);
void visit ( LIST_SEM *s );


int main ( int argc , char *argv[] ){


	srand(time(NULL));

	int 		K;
	int		t_id;
	int		priority;
	int		N;
	char		*file_name;
	char		buffer[BUFFER];


	//get number of threads
	K = atoi ( argv[1] );
	file_name = argv[2];
	
	
	//thread creation
	pthread_t *th = ( pthread_t * ) malloc ( K * sizeof ( pthread_t ) );

	//thread data structure
	THREAD_DATA *th_data = ( THREAD_DATA * ) malloc ( K * sizeof ( THREAD_DATA ) ); 


	//priority semaphore
	PRIORITY_SEMAPHORE *ps = ps_init ( 0 );

	
	//pipe used by the threads to comunicate with the main thread
	int ppipe[2];
	pipe ( ppipe );

	
	for ( int i = 0; i < K; i++ ){

		th_data[i].id = i;
		th_data[i].class = rand() %3; //[0,2] C1 = 0; C2 = 1; C3 = 2;
		th_data[i].p_sem = ps;
		th_data[i].ppipe = ppipe;

		pthread_create ( &th[i] , NULL , th_work ,   &th_data[i] );

	}


	//opening file log
	int fd = open ( file_name , O_WRONLY | O_CREAT , 0777);

	while ( 1 ){

		read ( ppipe[0] , &t_id , sizeof ( int ));
		read ( ppipe[0] , &priority , sizeof ( int ));
		read ( ppipe[0] , &N , sizeof (int));
		
		sprintf ( buffer , "%d %d %d\n"  , t_id , priority , N );
		write ( fd , buffer , strlen ( buffer ));
		memset ( buffer , 0 , BUFFER );
	
		if ( N == 0 )
			ps_signal ( ps );
	}
	

	for ( int i = 0; i < K; i++ )
		pthread_join ( th[i] , NULL );


	return 0;
}


void *th_work ( void *param ){

	
	THREAD_DATA *d = ( THREAD_DATA * ) param ;
	
	int status = 1;

	while ( 1 ){

		status = 0;
		write ( d->ppipe[1] , &d->id , sizeof ( int ));
		write ( d->ppipe[1] , &d->class , sizeof ( int ));
		write ( d->ppipe[1] , &status , sizeof ( int ));
	
		//waiting for enter in the critical region
		ps_wait ( d->p_sem  , d->class );


		status = 1;
		write ( d->ppipe[1] , &d->id , sizeof ( int ));
		write ( d->ppipe[1] , &d->class , sizeof ( int ));
		write ( d->ppipe[1] , &status , sizeof ( int ));	

	
		sleep( ( rand() %9 ) + 2 ); //wait [2,10] seconds

	}
	

	return NULL;
}



PRIORITY_SEMAPHORE * ps_init ( int value ){


  	PRIORITY_SEMAPHORE *s;

  	s = (PRIORITY_SEMAPHORE *) malloc (sizeof (PRIORITY_SEMAPHORE));
  	s->cnt = value;
  	s->mutex = (sem_t *) malloc (sizeof (sem_t));
  	sem_init (s->mutex, 0, 1);
  	s->sem_list = (LIST_SEM *) malloc (sizeof (LIST_SEM));      // create empty queue with sentinels
  	s->sem_list->priority = 100000;
  	s->sem_list->next = (LIST_SEM *) malloc (sizeof (LIST_SEM));
  	s->sem_list->next->priority = -1;
  	s->sem_list->next->next = NULL;
  	
	return s;

}


int ps_wait (PRIORITY_SEMAPHORE * s, int prio){

  	LIST_SEM 	*e;
  	int 		priority = 0;

  	sem_wait (s->mutex);
  	if (--s->cnt < 0) {

	    e = enqueue_sorted (s->sem_list, prio);
	    sem_post (s->mutex);
	    sem_wait (e->sem);
	    sem_destroy (e->sem);
	    priority = e->priority;
	    free(e);

	 }
	 else
	   sem_post (s->mutex);
	
	return priority;
}


void ps_signal (PRIORITY_SEMAPHORE * s){

	sem_t 		*sem;
	LIST_SEM 	*e;
	  
	sem_wait (s->mutex);
	if (++s->cnt <= 0) {

	  e = dequeue_sorted (s->sem_list);
	  sem_post (e->sem);

	}
	sem_post (s->mutex);

	 return;
}

void visit ( LIST_SEM *s ){


	LIST_SEM 	*p = s;

	while ( ( p = p->next ) != NULL )
		fprintf ( stdout , "%d " , p->priority );

	fprintf ( stdout , "\n\n" ); 

	return;
}


LIST_SEM *enqueue_sorted (LIST_SEM * head, int priority){

	LIST_SEM *p, *new_elem;

	p = head;                   // search
	while (priority <= p->next->priority)
		p = p->next;

		

	new_elem = (LIST_SEM *) malloc (sizeof (LIST_SEM));
	new_elem->sem = (sem_t *) malloc (sizeof (sem_t));
	sem_init (new_elem->sem, 0, 0);
	new_elem->priority = priority;
	new_elem->next = p->next;
	p->next = new_elem;

	return new_elem;
}

LIST_SEM *dequeue_sorted (LIST_SEM * head){

	LIST_SEM *p;
	p = head->next;
	head->next = head->next->next;
	return p;
}




