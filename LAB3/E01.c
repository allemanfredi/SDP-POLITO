#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

typedef struct ThreadData{
	
	int left;
	int right;

}ThreadData;

int v[100];
ThreadData data;
pthread_t thread;
int size = 1;
int dim = 0;




void swapp(int i, int j){
	int tmp;
	tmp = v[i];
	v[i] = v[j];
	v[j] = tmp;
}


void *quicksort ( void *arg ) {
	
	ThreadData *d = ( struct ThreadData * ) arg;
	
	for ( int j = 0; j < dim; j++ )	
		printf ( "%d " , v[j] ); 
	printf ( "\n");

	
	int i, j, x, tmp;
	if (d->left >= d->right) return NULL;
	x = v[d->left];
	i = d->left - 1;
	j = d->right + 1;
	while (i < j) {
		while (v[--j] > x);
			while (v[++i] < x);
				if (i < j)
					swapp (i,j);
	}
	
	ThreadData d1 , d2;
	d1.left = d->left;
	d1.right = j;

	d2.left = j + 1;
	d2.right = d->right;
	
	if ( (d->right - d->left > size) ){
	
		
		pthread_t t1 , t2;
		pthread_create ( &t1 , NULL , quicksort , &d1 );
		pthread_create ( &t2 , NULL , quicksort , &d2 );
		
		pthread_join (t1 , NULL );
		pthread_join (t2 , NULL );	
		
	} 
	else {
		quicksort ( &d1 ); 
		quicksort ( &d2 ); 
	
	}
	return NULL;
	
}




int main ( int argc , char *argv[] ){
	
	size = atoi ( argv[1] );
	FILE *f;
	if (  ( f = fopen( "v1.b" , "rb" ) )  == NULL ){ printf ( "ERROR FILE\n"); return 0; }
	
	int i = 0;
	int app;
	while ( fread ( &v[i] , sizeof( int ) , 1 , f ) > 0 ){		
		printf ( "%d \n" , v[i] );
		i++;	
	}
	dim = i;
	data.left = 0;
	data.right = i-1;
	pthread_create ( &thread , NULL , quicksort , &data );
	
	
	pthread_join ( thread , NULL );
	//sleep ( 1000 );
	for ( int j = 0; j < dim; j++ )	
		printf ( "%d " , v[j] ); 
	
	printf ( "\n");

	return 0;
}
