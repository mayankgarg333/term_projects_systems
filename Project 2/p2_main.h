/**********************************************************************                        
                                                                                               
   File          : p2_main.h
                                                                                               
   Description   : Project #2 Main Header
                                                                                               
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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define RTHREADS 5   // Number of reader threads
#define QLENGTH 2    // Max number of jobs queued

pthread_mutex_t mut;                 // lock for synchronizing access to the job queue
pthread_mutex_t sign_mutex;          // lock for synchronizing access to signatures
pthread_cond_t condc;                // condition variable for consumer
pthread_cond_t condp;                // condition variable for producer

/* file scanning status */
typedef struct
{
	char *file_name;             // name of file  
	int fd;                      // file index
	int writing;                 // being written?
	pthread_rwlock_t *lock;      // read-write lock for file
	struct file_node* next;      // next file in chain of file nodes
} file_node;

file_node* file_map_list;            // file node list
int total_cnt;                       // number of files

/* job info */
typedef struct
{
	int loc;                     // location for fix
	int offset;                  // length of fix
	file_node *fnode;            // file node to be fixed  
} node;

/* queue */
typedef struct 
{ 
	node *buf[QLENGTH];          // jobs
	int head, tail;              // tracking list
	int full, empty;             // flags
} queue;

node* new_node(file_node*, int, int);
queue *queue_init (void);
int queue_add (queue *, node*);
node* queue_delete (queue *);

void *thread_reader(void *);
void do_read(file_node*, char*, int, queue*, pthread_rwlock_t*);
void write_queue(file_node*, int, int, queue*, pthread_rwlock_t*);

void *thread_writer(void *);
node* read_queue(queue*);
void do_write(node*);

void init_locks();
int read_dir(char*);
void create_file_map(char*);

extern char *dirname;
extern char *signature[];
extern int sign_idx;
extern int read_over;
extern int wthreads; 
