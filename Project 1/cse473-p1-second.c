
/**********************************************************************

   File          : cse473-p1-second.c

   Description   : This is second chance page replacement algorithm
                   (see .h for applications)
                   See http://www.cs.cf.ac.uk/Dave/C/node27.html for info

   By            : Trent Jaeger, Yuquan Shan

***********************************************************************/
/**********************************************************************
Copyright (c) 2016 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this softwiare without specific prior written permission.

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

/* Include Files */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>

/* Project Include Files */
#include "cse473-p1.h"

/* Definitions */

/* second chance list */

typedef struct second_entry {  
  int pid;
  ptentry_t *ptentry;
  struct second_entry *next;
  struct second_entry *prev;
} second_entry_t;

typedef struct second {
  second_entry_t *first;
} second_t;

second_t *page_list;

/**********************************************************************

    Function    : init_second
    Description : initialize second-chance list
    Inputs      : fp - input file of data
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int init_second( FILE *fp )
{
  page_list = (second_t *)malloc(sizeof(second_t));
  page_list->first = NULL;
  return 0;
}


/**********************************************************************

    Function    : replace_second
    Description : choose victim based on second chance algorithm (first with ref == 0)
    Inputs      : pid - process id of victim frame 
                  victim - frame assigned from fifo -- to be replaced
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int replace_second( int *pid, frame_t **victim )
{
  /* Task #3 */
  second_entry_t *tmp;
  tmp=page_list->first->next;
  printf("IN the Replace Second \n");  
//  printf("Checking all pointers\n %x %x %x %x %x\n", tmp, tmp->next, tmp->next->next,tmp->next->next->next, tmp->next->next->next->next);
  //printf("Checking all pointers\n %x %x %x %x %x\n", tmp, tmp->prev, tmp->prev->prev,tmp->prev->prev->prev, tmp->prev->prev->prev->prev);

  while(tmp->ptentry->bits & REFBIT){
	printf("Inside While loop current fram id =%d\n",tmp->ptentry->frame);
	tmp->ptentry->bits=tmp->ptentry->bits & (~REFBIT);
  	tmp=tmp->next;
  }
  *pid=tmp->pid;
  int frame_number=tmp->ptentry->frame;
  printf("victim pid =%d, victim frame =%d \n",*pid,frame_number);
  *victim=&physical_mem[frame_number];

  tmp->prev->next=tmp->next;
//  printf("done 1\n");
  tmp->next->prev=page_list->first;
//  printf("done 2\n");
//  printf("page_list->first =%x, tmp->prev= %x\n",page_list->first,tmp->prev);
  page_list->first=tmp->prev;
//  printf("Done address tranfer\n");
  return 0;
}


/**********************************************************************

    Function    : update_second
    Description : update second chance on allocation 
    Inputs      : pid - process id
                  f - frame
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int update_second( int pid, frame_t *f )
{
  /* Task #3 */
  second_entry_t *list_entry= (second_entry_t *) malloc(sizeof(second_entry_t));
  list_entry->pid=pid;
  int page;
  page=f->page;
  list_entry->ptentry=&processes[pid].pagetable[page];
  list_entry->next=NULL;
  list_entry->prev=NULL;

  printf("UPDATE second \n");
  
  if(page_list->first==NULL){
        printf("updated second, added mapping at the first, page =%d ---- frame = %d \n",page,f->number);
        page_list->first=list_entry;
        list_entry->next=page_list->first;
	list_entry->prev=page_list->first;
        
  }
  else{
    // add at the next of the first
         printf("updated second, added mapping, page =%d ---- frame = %d \n",page,f->number);
      	 page_list->first->next->prev=list_entry;
    	 list_entry->next=page_list->first->next;
    	 page_list->first->next=list_entry;
    	 list_entry->prev=page_list->first;
    	 page_list->first=list_entry;
 	   
}
return 0;
}








