
/**********************************************************************

   File          : cse473-p1-mfu.c

   Description   : This is most frequently used replacement algorithm
                   (see .h for applications)

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

typedef struct mfu_entry{  
  int pid;
  ptentry_t *ptentry;
  struct mfu_entry *next;
  struct mfu_entry *prev;
} mfu_entry_t;

typedef struct mfu{
  mfu_entry_t *first;
} mfu_t;

mfu_t *page_list;

/**********************************************************************

    Function    : init_mfu
    Description : initialize mfu list
    Inputs      : fp - input file of data
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int init_mfu( FILE *fp )
{
  page_list = (mfu_t *)malloc(sizeof(mfu_t));
  page_list->first = NULL;
  return 0;
}


/**********************************************************************

    Function    : replace_mfu
    Description : choose victim based on mfu algorithm, take the frame 
                  associated the page with the largest count as victim 
    Inputs      : pid - process id of victim frame 
                  victim - frame assigned from fifo -- to be replaced
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int replace_mfu( int *pid, frame_t **victim )
{
  /* Task 3 */

  mfu_entry_t *tmp;
  tmp=page_list->first;

  int max_ct=0;
  int i;int num;
  for(i=0;i<PHYSICAL_FRAMES;i++){
	 printf(" In replace mfu, page ----%d  frame ---- %d ---- ct %d pid---%d\n",tmp->ptentry->number, tmp->ptentry->frame,tmp->ptentry->ct, tmp->pid);
	 if(tmp->ptentry->ct>max_ct){
		max_ct=tmp->ptentry->ct;
		num=i;
		  *pid=tmp->pid;
 		  int frame_number=tmp->ptentry->frame;
		  *victim=&physical_mem[frame_number];
          }
  tmp=tmp->next;
  }
  printf("num  =%d \n",num);
  // removing the victim
  if(num==0){
  	page_list->first=page_list->first->next;  
        return 0;
  }

  
 
  tmp=page_list->first;
  for(i=0;i<num;i++){
	tmp=tmp->next;
  }

  if(num==PHYSICAL_FRAMES-1){
	tmp->prev->next=NULL;
  }
  else{
  tmp->prev->next=tmp->next;
  tmp->next->prev=tmp->prev;
  }

  return 0;
}

/**********************************************************************

    Function    : update_mfu
    Description : create container for the newly allocated frame (and 
                  associated page), and insert it to the end (with respect
                  to page_list->first) of page list 
    Inputs      : pid - process id
                  f - frame
    Outputs     : 0 if successful, -1 otherwise

***********************************************************************/

int update_mfu( int pid, frame_t *f )
{
  /* Task 3 */
  mfu_entry_t *list_entry= (mfu_entry_t *) malloc(sizeof(mfu_entry_t));
  list_entry->pid=pid;
  int page;
  page=f->page;
  list_entry->ptentry=&processes[pid].pagetable[page];
  list_entry->next=NULL;
  list_entry->prev=NULL;

  printf("UPDATE MFU \n"); 
  if(page_list->first==NULL){
        printf("updated mfu, added mapping at the first, page =%d ---- frame = %d \n",page,f->number);
	page_list->first=list_entry;
	return 0;
  }
  
  mfu_entry_t *tmp;
  tmp=page_list->first;
  /* if it is fist in the list */
  int i;
  for(i=0;i<PHYSICAL_FRAMES-1;i++){
	
 	 if(tmp->next==NULL){
		printf("updated mfu, added mapping at page=%d --- frame =%d \n",page, f->number);
  		tmp->next=list_entry;
		tmp->next->prev=tmp;
		break;
	  }
  tmp=tmp->next;


}

return 0;
}
