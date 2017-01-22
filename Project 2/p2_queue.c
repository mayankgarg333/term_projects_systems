/**********************************************************************                        
                                                                                               
   File          : p2_queue.c                                                               
                                                                                               
   Description   : Queue management
                                                                                               
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

queue *queue_init (void)
{   
    queue *q;

    q = (queue *)malloc (sizeof (queue));
    if (q == NULL) 
    {
        return (NULL);
    }
    
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    
    return (q);
}

node* new_node(file_node* fnode, int loc, int offset)
{
    node * n;

    n = (node*) malloc (sizeof (node));
    if (n == NULL)
    {
        return NULL;
    }

    n->loc = loc;
    n->offset = offset;
    n->fnode = fnode;

    return (n);
}

int queue_add (queue *q, node *n)
{
    if (q->full == 0)
    {
        q->buf[q->tail] = n;
        q->tail++;
        if (q->tail == QLENGTH)
        {
            q->tail = 0;
        }
        if (q->tail == q->head)
        {
            q->full = 1;
        }
        
        q->empty = 0;

        return 0;
    }
    else
    {
        return -1;
    }
}

node* queue_delete (queue *q)
{
    node *out;
    if (q->empty == 0)
    {
        out = q->buf[q->head];
        q->head++;

        if (q->head == QLENGTH)
        {
            q->head = 0;
        }
        if (q->head == q->tail)
        {
            q->empty = 1;
        }
    
        q->full = 0;

        return out;
    }
    else
    {
        return NULL;
    }
}
