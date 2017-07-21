#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "semaphore.h"

#define NUM_THREADS 2

pthread_t client1 , client2 ;
FILE *fv1 , *fv2;
int g;
int  stop = 0;
int request;

pthread_mutex_t lock , lock2;

FILE *file;

sem_t *w , *r , *ok , *stopp;



static void *read_file ( void *arg ){
    
    FILE *f = ( FILE * ) arg;
    int app;
    
    while ( fread ( &app , sizeof( int ) , 1 , f ) > 0 ){
        
        sem_wait ( w );  // i'm waiting that no other client is writing
        g = app;
        sem_post ( ok ) ;// i comunicate to the server that it can modify g
        sem_wait ( r ); ;// i'm waiting that the client has just red
        printf ( "Current value %d of G , Previous : %d\n " , g , app );
    
        sem_post ( w );

        
    }
    
    stop++;
    if (stop == 2){
    sem_post(ok);
    }

    pthread_exit(NULL);
    
}




int main ( int argc , char *argv[] ){
    
    if  ( ( fv1 = fopen ( "v1.b" , "rb") ) == NULL ){
        printf ( "error during the v1.b opening\n" );
        return -1;
    }
    
    if  ( ( fv2 = fopen ( "v2.b" , "rb") ) == NULL ){
        printf ( "error during the v2.b opening\n" );
        return -1;
    }
    
    pthread_mutex_init ( &lock , NULL );
    pthread_mutex_init ( &lock2 , NULL );
				
    w = (sem_t *) malloc(sizeof(sem_t));
    r = (sem_t *) malloc(sizeof(sem_t));
    ok = (sem_t *) malloc(sizeof(sem_t));
    stopp = (sem_t *) malloc(sizeof(sem_t));
    
    
    
    if ( ( sem_init (w, 0, 1) ) || ( sem_init (r, 0, 0) ) || ( sem_init (ok, 0, 0) ) || ( sem_init (stopp, 0, 0) ))
        printf ( "Error during the semaphore initilization\n ");
    
    
    pthread_create ( &client1 , NULL , read_file , ( void * ) fv1 );
    pthread_create ( &client2 , NULL , read_file , ( void * ) fv2 );
    
    while ( stop != NUM_THREADS ){
        
        sem_wait ( ok ); // i'm waiting for the "ok"  
        g *= 3;
        request++;
        sem_post ( r );
        
        
    }
    
    
    printf ( "Number of request %d\n" , request - 1);
    
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lock2);
        
    pthread_join(client1, NULL);
    pthread_join(client2, NULL);
    fclose ( fv1 );
    fclose ( fv2 );
    
    
    
    
    return 0;
}
