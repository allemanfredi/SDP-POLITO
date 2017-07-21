#include "semaphore.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "spinlock.h"



#define TRUE 1
#define FALSE 0
#define NULL '\0'


struct semaphore{
	
	int sem;
	uint id;
	struct spinlock lock;
	

};

struct table{
	
	struct semaphore v_sem[MAXSEMAPHORE];
	struct spinlock lock;
	int semaphore_count;
	int vet_assigned[MAXSEMAPHORE];
	
	
};


struct table table;

int sem_alloc (){
	
	struct semaphore *sem;
	acquire(&table.lock);

	if ( table.semaphore_count < MAXSEMAPHORE ){
		if ( ( sem = ( struct semaphore * ) kalloc () ) == 0 ) // if the initialization missed i have to release the memory 
			goto bad;
	
		if ( table.vet_assigned[table.semaphore_count] == 0 ){
			sem->sem = 0;
			sem->id = table.semaphore_count;
			initlock( &sem->lock, "semaphore");

			table.vet_assigned[table.semaphore_count] = 1;
	
			int index = table.semaphore_count;
			table.v_sem[table.semaphore_count] = *sem;
			table.semaphore_count += 1;
			release(&table.lock);
			return index;
		}
	}

	bad:
	kfree ( ( char * ) sem );
	release(&table.lock);
	return -1;


	
}



	

void sem_init ( int sem , int count ){
	
	
	acquire(&table.v_sem[sem].lock);

	if ( table.vet_assigned[sem] == 1 )
        	table.v_sem[sem].sem= count;
	else
		panic ( "semaphore non attisgned\n" );
	
	release(&table.v_sem[sem].lock);

}

void sem_destroy ( int sem ){
	
	acquire(&table.lock);

	if ( table.vet_assigned[sem] == 1 ){	
		kfree((char*)&table.v_sem[sem]);
		table.semaphore_count--;
		table.vet_assigned[sem] = 0;
	}
	
	release(&table.lock);
	 

}

void sem_wait ( int sem ){
	

	
	acquire ( &table.v_sem[sem].lock );
	
	if ( table.vet_assigned[sem] == 1 ){

		table.v_sem[sem].sem--;
		if ( table.v_sem[sem].sem < 0 )
			sleep( &table.v_sem[sem].id , &table.v_sem[sem].lock );
	}

	release ( &table.v_sem[sem].lock );
		
	return;	
}

void sem_post ( int sem ){

	
	acquire ( &table.v_sem[sem].lock );
	
	if ( table.vet_assigned[sem] == 1 ){	

		table.v_sem[sem].sem++;
		if ( table.v_sem[sem].sem >= 0 )
			wakeup( &table.v_sem[sem].id );

	}

	release ( &table.v_sem[sem].lock );
		
	return;
}

