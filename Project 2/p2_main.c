/**********************************************************************                        
                                                                                               
   File          : p2_main.c                                                               
                                                                                               
   Description   : Project #2 Main 
                                                                                               
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

#include <sys/types.h>
#include <dirent.h>
#include "p2_main.h"

char *signature[] = {"the", "but", "rat", "and", "far"};  // signatures to replace
char *dirname;       // directory name where files to be scanned are - scan all in directory
int sign_idx;        // index to signatures
int read_over;       // reading (scanning) is complete 
int wthreads = 0;    // count for number of writer threads

/******************************************************

    Function    : main
    Description : this is the main function for the project
    Inputs      : argc - number of command line parameters
                  argv - the text of the arguments (input directory)
    Outputs     : 0 if successful, -1 if failure

 ******************************************************/

int main(int argc, char* argv[])
{
	int i;

	pthread_t read_thread[RTHREADS];          
	pthread_t *write_thread;

	if (argc < 3) 
	{
		printf("Usage: %s <Input file> <number_of_writer_threads> \n", argv[0]);
		return 1;
	}

	/* create writer threads based on input */
	wthreads = atoi(argv[2]);
	if ((wthreads >= 0) && (wthreads < 3))
		write_thread = (pthread_t *)malloc(sizeof(pthread_t)*wthreads);

	/* read in test files */
	dirname = argv[1];
	if (1 == (read_dir(argv[1]))) 
	{
		return 1;
	}
    
	init_locks();

	queue *q = queue_init();

	/* create reader threads */
	for(i=0; i < RTHREADS; i++)
	{
		// Task #1 - Create reader threads
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&read_thread[i],&attr,thread_reader,q);

	}
    
	/* create writer threads */
	for(i=0; i < wthreads; i++)
	{
		// Task #1 - Create writer threads
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_create(&write_thread[i],&attr,thread_writer,q);


	}

	/* wait for reader threads to terminate */
	for(i=0; i < RTHREADS; i++)
	{
		// Won't work until Task #1 is completed
		pthread_join( read_thread[i], NULL);
	}

	printf("\nReaders done\n");

	/* then can terminate writer threads */
	for(i=0; i < wthreads; i++)
	{
		// Won't work until Task #1 is completed
		pthread_kill(write_thread[i], SIGALRM);
	}

	printf("\nWriters done\n");

	exit(0);
}
