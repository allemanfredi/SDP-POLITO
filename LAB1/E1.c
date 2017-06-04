#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int n1 , n2;
int *v1 = NULL;
int *v2 = NULL;
int i;

void quick_sort ( int *a , int l , int r );
int partition ( int *a , int l , int r );
void print_vector ( int *vet , int dim );

int main ( int argc , char *argv[] ){

	srand ( time ( NULL ) );
	
	printf ( "Give me N1 and N2 \n" );
	scanf ( "%d%d", &n1 , &n2 );
	
	v1 = ( int * )  malloc ( n1 * sizeof ( int ) ); 
	v2 = ( int * )  malloc ( n2 * sizeof ( int ) ); 
	
	for ( i = 0; i < n1 ; i++ )
		v1[i] = ( rand() % 91 ) + 10; // [10-100]
		
	
	for ( i = 0; i < n2 ; i++ )
		v2[i] = (rand() % 82) + 20; // [21-101]
	
	
	
	/*quick_sort ( v1 , 0 , n1 - 1 );
	quick_sort ( v2 , 0 , n2 - 1 );*/
	
	printf ( "v1 : " );
	print_vector( v1 , n1 );
	
	printf ( "v2 : " );
	print_vector( v2 , n2);
	
	
	FILE *fv1; // file descriptor1
	if ((fv1 = fopen("v1.txt", "w" )) == NULL){
       fprintf(stderr," error open v1.txt");
		return 0; 
	}
	for ( i = 0; i < n1; i++)
		fprintf ( fv1 , "%d" , v1[i] );
	fclose ( fv1 );
	
	
	
	FILE *fv2; // file descriptor2
	if ((fv2 = fopen("v2.txt", "w" )) == NULL){
       fprintf(stderr," error open v2.txt");
		return 0; 
	}
	for ( i = 0; i < n2; i++)
		fprintf ( fv2 , "%d" , v2[i] );
	fclose ( fv2 );
	
	
	//binary file1
	if ((fv1 = fopen("v1.b", "wb" )) == NULL){
       fprintf(stderr," error open v1.b");
		return 0; 
	}
	
	for ( i = 0; i < n1; i++)
    	fwrite ( &v1[i] , sizeof(int) , 1 , fv1 );
	fclose(fv1);
	

	
	
	if ((fv2 = fopen("v2.b", "wb" )) == NULL){
       fprintf(stderr," error open v2.b");
		return 0; 
	}
	for ( i = 0; i < n2; i++)
    	fwrite ( &v2[i] , sizeof(int) , 1 , fv2 );
	fclose(fv2);
	
	

	return 0;

}


void print_vector ( int *vet , int dim ){

	i = 0;
	for ( ; i < dim; i++ )
		printf ( "%d " , vet[i]);
	printf ( "\n");
	
	return;

}


void quick_sort( int *a, int l, int r)
{
   int j;

   if( l < r ) 
   {
   	// divide and conquer
        j = partition( a, l, r);
       quick_sort( a, l, j-1);
       quick_sort( a, j+1, r);
   }
	
}


int partition( int *a, int l, int r) {
   int pivot, i, j, t;
   pivot = a[l];
   i = l; j = r+1;
		
   while( 1)
   {
   	do ++i; while( a[i] <= pivot && i <= r );
   	do --j; while( a[j] > pivot );
   	if( i >= j ) break;
   	t = a[i]; a[i] = a[j]; a[j] = t;
   }
   t = a[l]; a[l] = a[j]; a[j] = t;
   return j;
}

