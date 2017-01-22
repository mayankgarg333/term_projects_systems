/**********************************************************************                        
                                                                                               
   File          : p2_thread_writer.c                                                               
                                                                                               
   Description   : This is the writer thread's code - overwrite matched "signatures"
                                                                                               
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

void term_fun(int signo)
{
	read_over = 1;
}

/******************************************************

    Function    : thread_writer
    Description : this is the main function for the writer thread
    Inputs      : Q - queue for readers to post work for writers
    Outputs     : in practice, none

 ******************************************************/

void *thread_writer(void *Q)
{
	queue* q = (queue*)Q;
	node* n;

	signal(SIGALRM, term_fun);
    
	printf("++ Writer Started: %ld\n", pthread_self());

	while(1)
	{
		/* find job in queue */
		n = read_queue(q);
        	
		/* do writing work - replace data at location */
		if (n != NULL)
		{
			do_write(n);
		}
	}
}


/******************************************************

    Function    : read_queue
    Description : Extract a job from the shared queue, if one is present
    Inputs      : Q - queue for readers to post work for writers
    Outputs     : a pointer to a node (file) or NULL

 ******************************************************/

node* read_queue(queue* q)
{
	node *n;

	// Task #7 - Writers 
	printf("++ Writer is checking queue: Thread number: %ld\n", 
	       pthread_self());
   
	pthread_mutex_lock(&mut);

	printf("** QUEUE entry ** : Writer Thread number: %ld\n", 
	       pthread_self());


	/* handle case if queue is empty */
	if (q->empty == 1)
	{   
		printf("** QUEUE Exit - Writer - empty (waiting): Thread number: %ld\n", 
		       pthread_self());

                while(q->empty==1 && read_over==0)
                        pthread_cond_wait(&condp,&mut);


		printf("** QUEUE Entry - Writer - not empty: Thread number: %ld\n", 
		       pthread_self());

		/* make sure thread completes when reading is done */
		/* leave at end of this "IF block */
		if ((read_over == 1) && (q->empty == 1))
		{
			pthread_cond_signal(&condp);
			printf("** QUEUE exit ** : Readers done, empty queue : Writer Thread number: %ld\n",pthread_self());
			pthread_exit(0);
		}
	}
    
	n = queue_delete(q);

	/* NOTE: queue may be empty even after signaling (other writer won race)
	   so no job for some writers */

	printf("** QUEUE exit ** : Writer Thread number: %ld\n", 
	       pthread_self());
	
	pthread_cond_signal(&condc);
        pthread_mutex_unlock(&mut);
	return n;
}


/******************************************************

    Function    : do_write
    Description : modify file
    Inputs      : node - a file
    Outputs     : none

 ******************************************************/

void do_write(node *n)
{
	int i = 0;
	char ch;
	char filename[50];
	FILE *wfp;
	// Readers-writers lock - per file
	pthread_rwlock_t* rwlock;

	// Task #8 - Writers obtain lock for writing file, then release
	
	rwlock=n->fnode->lock;
	pthread_rwlock_wrlock(rwlock);
	

	printf("** WRITE Entry ** - Writer Thread number: %ld\n", pthread_self());
	printf("filename = %s, loc = %d, offset = %d\n",
	       n->fnode->file_name, n->loc, n->offset);
   
	memset(filename, '\0', 50 * sizeof(char));
	strcat(filename, dirname);
	strcat(filename, "/");
	strcat(filename, n->fnode->file_name); 

	wfp = fopen(filename, "r+");
	fseek( wfp, n->loc, SEEK_SET );

	while(i < n->offset)
	{
		ch = fgetc(wfp);
		fseek( wfp, -1, SEEK_CUR );
		fputc(toupper(ch), wfp);
		i++;
	}
    
	fclose(wfp);

	printf("** WRITE Exit ** - Writer Thread number: %ld\n", pthread_self());
	printf("filename = %s, loc = %d, offset = %d\n",
	       n->fnode->file_name, n->loc, n->offset);
        pthread_rwlock_unlock(rwlock);

}


