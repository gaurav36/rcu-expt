#include <stdio.h>      
#include <pthread.h>    
#include <string.h>
#include <urcu-bp.h>  
#include <urcu/rculist.h>
#include <pthread.h>

// In run script, please define  locking mechanism which one you want to use.
// If you want use other mechanism  please define  new locking mechanism.(No need to modify code ).

#ifdef USE_SPINLOCK
	pthread_spinlock_t 		lock;
	#define   LOCK(lock)      	pthread_spin_lock(&lock);
        #define   UNLOCK(lock) 		pthread_spin_unlock(&lock);
#else
	pthread_mutex_t 		lock;
        #define   LOCK(lock) 		pthread_mutex_lock(&lock);
        #define   UNLOCK(lock)		pthread_mutex_unlock(&lock);
#endif


CDS_LIST_HEAD(ghead);

void list_adder( void *data);
void list_remover(void *data);
void list_printer (void *data);
void increase_sleep (void *data);
void update_nodes (void *data);

int sleep_val= 0;

struct mynode
{
	int value;
	struct cds_list_head link;
//	struct rcu_head rcu_head;	/* for call_rcu() */
};

void list_printer (void *data)
{
	struct mynode *iter = NULL;
	while (1) 
	{
		rcu_read_lock();

		printf("\n\n\n.........List is ......\n");
		cds_list_for_each_entry_rcu (iter, &ghead, link) 
		{
			
			printf (" %d  ", iter->value);
		}
		printf("\n");
		sleep(sleep_val);
		
		rcu_read_unlock();
	}

}

void update_nodes (void *data)
{

	struct mynode *node,*n;
	
	while (1)
	{
		///here .. need some synchronisation  mechanism  if update_nodes is called by multiple threads to avoid mutliple writes at a time  .
		// Mutual exclusion required between update, remove node and add .
		LOCK(lock);

		cds_list_for_each_entry_safe(node, n, &ghead, link) 
		{

			sleep(sleep_val);

			struct mynode *new_node;

			new_node = malloc(sizeof(*node));
			if (!new_node) 
			{
				return ;
			}
			/* Replacement node value is negated original value. */
			new_node->value = node->value * -1 ;              // just making -ve num to postive and vice-versa
			
			printf("\nnew_node->value =%d, node->value done =%d\n", new_node->value,node->value);

			cds_list_replace_rcu(&node->link, &new_node->link);
			

			synchronize_rcu();    // you can replace  call_rcu  for non blacking  &do free in call back function 

			free(node);   // free the memory of old node 
			

		}

		UNLOCK(lock);
		// end of  synchronisation
	}

}

void  list_adder( void *data)
{
	struct mynode *node;
	int val=1;


	while(1)
	{
		LOCK(lock);

		node = calloc(sizeof(*node), 1);
		if (!node)
			return ;

		node->value = val++;
		cds_list_add_tail_rcu(&node->link, &ghead);
		printf("\none more node added to list with val =%d ",node->value );

		UNLOCK(lock);

		sleep(sleep_val);
	}
	return ;
}

void list_remover(void *data)
{
	struct mynode *node,*n;

	while(1)
	{      
		sleep(sleep_val+10);

		LOCK(lock);

		cds_list_for_each_entry_safe(node, n, &ghead, link) 
		{	
				
			cds_list_del_rcu(&node->link);
			synchronize_rcu();
			printf("\n\n>>>>>>>>>>node is deleted which has data =%d\n\n",node->value );
			free(node);	
		}

		UNLOCK(lock);

	}

}

void increase_sleep (void *data)
{

	while(1)
	{	
 	  getchar();
	  getchar();
	  if(sleep_val==0)
  	 	 sleep_val=7;
	else
	    sleep_val=0;

		printf("\n\n\nsleep val=%d",sleep_val);
	}
}

int main ()
{
	pthread_t thread1, thread2, thread3,thread4,thread5;  

	pthread_create (&thread1, NULL, (void *) list_adder, NULL);
	pthread_create (&thread2, NULL, (void *) list_remover, NULL);
	pthread_create (&thread3, NULL, (void *) list_printer,NULL);
	pthread_create (&thread4, NULL, (void *) update_nodes,NULL);
	pthread_create (&thread5, NULL, (void *) increase_sleep,NULL);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);
	pthread_join(thread4, NULL);
	pthread_join(thread5, NULL);
 
       return 0;
}
