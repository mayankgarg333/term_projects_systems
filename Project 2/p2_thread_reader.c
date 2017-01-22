/**********************************************************************                        
                                                                                               
   File          : p2_thread_reader.c                                                               
                                                                                               
   Description   : This is the reader's code - scans files for "signatures"
                                                                                               
   By            : Aakash Sharma and Trent Jaeger

***********************************************************************/
/**********************************************************************                                                          
Copyright (c) 2016 The Pennsylvania State University

All rights reserved.
                                                                                                                                 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following condi tions
are met:
                                                                                                                                 
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

* Neither the name of The Pennsylvania State University nor the names
of its contributors may be used to endorse or promote products
derived from this softwiare without specific prior written permission.
                                                                                                                                 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#include <time.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "p2_main.h"


void print_list(file_node *list)
{
	file_node *itr;
	for(itr=list; itr!= NULL; itr=itr->next)
	{
		printf("file: %s, Thread: %ld\n", itr->file_name, pthread_self());
	}
}
 
file_node *get_nth_node (file_node** local_list, int index) 
{
	file_node *node, *tmp, *next_node;
	int i = 1;
	node = *local_list;
	next_node = node->next;

	printf("index  = %d\n", index);

	for (node = *local_list, next_node = node->next; 
	     next_node != NULL ; 
	     node = node->next, next_node = next_node->next, i++) 
	{
		if (index == i+1) 
		{
			return node;
		}
	}
}
 
void shuffle_list (file_node** local_list) 
{
	file_node *node, *tmp;
	int idx;
	int cnt = total_cnt;
	while (cnt > 0) 
	{
		idx = rand() % cnt;
		if (idx == 0)
		{
			tmp = *local_list;
			*local_list = (*local_list)->next;
			tmp->next = (*local_list)->next;
			(*local_list)->next = tmp;
		}
		else
		{
			node = get_nth_node(local_list, cnt);

			tmp = node->next;
			node->next = tmp->next;
			tmp->next = *local_list;
			*local_list = tmp;
		} 

		cnt--;
	}
}

void copy_list(file_node *src, file_node **dest)
{
	file_node *itr, *tmp;
	for(itr=file_map_list; itr!= NULL; itr=itr->next)
	{
		tmp = (file_node*) malloc(sizeof(file_node));
		tmp->file_name = itr->file_name;
		tmp->fd = itr->fd;

		tmp->lock = itr->lock;

		if(*dest == NULL)
		{
			tmp->next = NULL;
			*dest = tmp;
		}
		else
		{
			tmp->next = *dest;
			*dest = tmp;
		}
	}

	for(itr=*dest; itr!= NULL; itr=itr->next)
	{
		if(itr->next == NULL)
		{
			break;
		}
	}
}

/******************************************************

    Function    : thread_reader
    Description : this is the main function for the reader thread
    Inputs      : Q - queue for readers to post work for writers
    Outputs     : in practice, none

 ******************************************************/

void* thread_reader(void *Q)
{
	int local_sign_idx;
	int sign_size;
	char *sign;

	queue *q = (queue*)Q;
	file_node* local_list = NULL, *itr = NULL;
	/* Readers-writers lock - per file */
	pthread_rwlock_t *rwlock = NULL;

	/* Printfs for Task #2 
	printf("** SIGN entry ** : Thread number: %ld\n", 
	       pthread_self());
	printf("** SIGN exit ** : Sign: %s, Thread number: %ld\n", 
	       signature[local_sign_idx], pthread_self());
	*/


	// Task #2 - mutual exclusion for obtaining "signature" to check for
	/* get index for the "signature" that this reader will look for */

        pthread_mutex_lock(&sign_mutex);

        printf("** SIGN entry ** : Thread number: %ld\n", 
               pthread_self());

	local_sign_idx = sign_idx;
	sign_idx++;

	printf("** SIGN exit ** : Sign: %s, Thread number: %ld\n", 
               signature[local_sign_idx], pthread_self());
	
	pthread_mutex_unlock(&sign_mutex);



       // TASK#2 DONE



	/* obtain "signature" for this thread */
	sign_size = strlen(signature[local_sign_idx]);
	sign = (char*) malloc(sign_size);
	strcpy(sign, signature[local_sign_idx]);
	printf("Reader Thread number: %ld, sign: %s\n", pthread_self(), sign);

	/* Mix up the file list from this thread's perspective */
	copy_list(file_map_list, &local_list);
	shuffle_list(&local_list);


	// Task #3 - Ensure that readers have read lock for do_read 
	/* scan files in "local_list" in order for "signature" */
	for(itr=local_list; itr!= NULL; itr=itr->next)
	{

		rwlock=itr->lock;
                pthread_rwlock_rdlock(rwlock);
		printf("** READ Entry ** - Reading file: %s, Sign: %s, Thread number: %ld\n", 
		       itr->file_name, sign, pthread_self());
		/* scan file "itr" for "signature" and add work to "q" */
		do_read(itr, sign, sign_size, q, rwlock);
		
		printf("** READ Exit ** - is yielding when done with file: file: %s, Sign: %s, Thread number: %ld\n", 
		       itr->file_name, sign, pthread_self());
		pthread_rwlock_unlock(rwlock);
	}

	pthread_exit(0);  
}

 
/******************************************************

    Function    : do_read
    Description : Perform scan operation across whole file - waiting after 1000 bytes
    Inputs      : fnode - file to be read
                  sign - signature
                  sign_size - size of signature in bytes
                  q - work queue for writers
                  rwlock - for releasing rwlock every 1000 bytes read
    Outputs     : none  

 ******************************************************/

void do_read(file_node *fnode, char *sign, int sign_size, queue *q, pthread_rwlock_t *rwlock)
{   
	char ch;
	int i = 0, j = 0;
	char filename[50];
    
	FILE *rfp;

	memset(filename, '\0', 50 * sizeof(char));
	strcat(filename, dirname);
	strcat(filename, "/");
	strcat(filename, fnode->file_name);

	rfp = fopen(filename, "r");
    
	while ((ch = fgetc(rfp)) != EOF)
	{
		j++;
		if(ch == sign[i])
		{
			i++;
			if(i == sign_size)
			{
				printf("== Queuing file: %s, Sign: %s, Thread number: %ld, at file index: %d\n", 
				       fnode->file_name, sign, pthread_self(), j-sign_size);
				
				// Task #5 - Reader yield read lock during queueing, then reobtain after 
				printf("** READ exit - do_read - Reader queueing: file: %s, at index: %d, Thread number: %ld\n", 
				       fnode->file_name, j-sign_size, pthread_self());	
	                        pthread_rwlock_unlock(rwlock);
				write_queue(fnode, j-sign_size, sign_size, q, rwlock);
	                        pthread_rwlock_rdlock(rwlock);
				printf("** READ entry - do_read - Reader queueing done: file: %s, at index: %d, Thread number: %ld\n", 
				       fnode->file_name, j-sign_size, pthread_self());	
			}
		}
		else
		{
			i = 0;
			if (ch == sign[i])
			{
				i++;
			}
		}

		// Task #4 - Reader pause and yield read lock, then reobtain lock after sleep
		/* after reading for 1000 bytes release the rdlock - let writers in */
		if (j %1000 == 0)
		{
			printf("** READ exit - do_read ** - Reader pausing: file: %s, Sign: %s, Thread number: %ld, at index: %d\n", 
			       fnode->file_name, sign, pthread_self(), j);
	                pthread_rwlock_unlock(rwlock);
			usleep(1000);
	                pthread_rwlock_rdlock(rwlock);

			printf("** READ entry - do_read ** - Reader after pause: file: %s, Sign: %s, Thread number: %ld, at index: %d\n", 
			       fnode->file_name, sign, pthread_self(), j);
		} 
	}

	fclose(rfp);
}


/******************************************************

    Function    : write_queue
    Description : Write "signature" match to the write queue - produce for writers
    Inputs      : fnode - file to be read
                  loc - location of match
                  offset - size of signature in bytes
                  q - work queue for writers
                  rwlock - needed to enact yield
    Outputs     : none   // TJ: return anything?

 ******************************************************/

void write_queue(file_node* fnode, int loc, int offset, queue* q, pthread_rwlock_t *rwlock)
{
	node *n;

	n = new_node(fnode, loc, offset);
	
	pthread_mutex_lock(&mut);  
	// Task #6 - readers queue jobs (if space) and signal writers
	printf("** QUEUE entry ** : Reader Thread number: %ld\n", 
	       pthread_self());

	// Task #6 - readers wait (releasing mutex) if queue is full 
	//           readers renter queue critical section when queue has a empty slot
	// add logic to handle this case

	
	if(q->full==1){
		printf("** QUEUE exit ** - full (waiting) : Reader Thread number: %ld\n", 
	       pthread_self());
		while(q->full==1)
			pthread_cond_wait(&condc,&mut);

		printf("** QUEUE entry ** - full (signalled) : Reader Thread number: %ld\n", 
	       pthread_self());
	}

	/* Add work to the queue */
	printf("Queueing... Reader Thread number: %ld\n", pthread_self());
	printf("file name = %s, loc = %d, offset = %d\n",fnode->file_name, loc, offset);
	queue_add((queue*)q, n);

	// Task #6 - done with queue
	printf("** QUEUE exit ** : Reader Thread number: %ld\n", 
	       pthread_self());

        pthread_cond_signal(&condp);
	pthread_mutex_unlock(&mut);

}
