/**********************************************************************                        
                                                                                               
   File          : p2_locks.c                                                               
                                                                                               
   Description   : Initialize locks for files in chosen directory
                                                                                               
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

#include "p2_main.h"
#include <dirent.h>
#include <string.h>

void init_locks()
{
	pthread_mutex_init (&mut, NULL);
	pthread_mutex_init (&sign_mutex, NULL);
	pthread_cond_init (&condc, NULL);
	pthread_cond_init (&condp, NULL);
}


int read_dir(char* dir_name)
{
    DIR* FD;
    struct dirent* in_file;
    file_node* itr = file_map_list;

    if (NULL == (FD = opendir (dir_name)))
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return 1;
    }

    while ((in_file = readdir(FD)))
    {
        if (!strcmp (in_file->d_name, "."))
            continue;
        if (!strcmp (in_file->d_name, ".."))
            continue;

        printf("file name: %s\n", in_file->d_name);
        create_file_map(in_file->d_name);
        total_cnt++;
    }

    for(itr = file_map_list; itr!= NULL; itr=itr->next)
    {
        printf("%s\n", itr->file_name);
    }

    return 0;
}


void create_file_map(char* filename)
{
    file_node* tmp = (file_node*) malloc(sizeof(file_node));
    tmp->file_name = filename;

    pthread_rwlock_t *rwlock = (pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
    pthread_rwlock_init( rwlock, NULL );
    tmp->lock = rwlock;
    
    if (file_map_list == NULL)
    {
        tmp->next = NULL;
        tmp->fd = 0;
        tmp->writing = 0;
        file_map_list = tmp;
    }
    else
    {
        tmp->fd = file_map_list->fd + 1;
        tmp->writing = 0;
        tmp->next = file_map_list;
        file_map_list = tmp;
    }
}


