
/**********************************************************************

   File          : cse473-p3.c

   Description   : This is the main file for the file system project
                   (see .h for applications)

***********************************************************************/
/**********************************************************************
Copyright (c) 2016 The Pennsylvania State University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of The Pennsylvania State University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

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
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Project Include Files */
#include "cse473-filesys.h"
#include "cse473-disk.h"
#include "cse473-list.h"
#include "cse473-util.h"

/* Definitions */
#define CLIENT_USAGE "USAGE: cse473-p3 <fs_file> <instruction_file>\n"
#define MAX_ARGS 5
#define MAX_LINE_SIZE 750

/* macros */

/* program variables */
char cmd_delims[] = {' ', '('};
char arg_delims[] = {","};


/* Functions */
int(*createfn)( char *name, unsigned int flags ) = fileCreate;
int(*openfn)( char *name, unsigned int flags ) = fileOpen;
void(*listfn)( void ) = listDirectory;
void(*closefn)( unsigned int fd ) = fileClose;
int(*readfn)( unsigned int fd, char *buf, unsigned int bytes ) = fileRead;
int(*writefn)( unsigned int fd, char *buf, unsigned int bytes ) = fileWrite;
int(*seekfn)( unsigned int fd, unsigned int index ) = fileSeek;
int(*checkpointfn)( void ) = fsJournalCheckpoint;


/**********************************************************************

    Function    : processInitialize
    Description : initialize this process structure
    Inputs      : none
    Outputs     : 0 if successful, other represents error or termination

***********************************************************************/

int processInitialize( void ) 
{
  /* create the process */
  fs->proc = (proc_t *)malloc(sizeof(proc_t));

  if ( fs->proc == NULL ) {
    errorMessage("processInitialize: process allocation failed");
    return -1;
  }
   
  /* create the process's per-process file table */
  fs->proc->fstat_table = (fstat_t **)malloc(PROC_FILETABLE_SIZE * sizeof(fstat_t *));

  if ( fs->proc->fstat_table == NULL ) {
    errorMessage("processInitialize: fstat_table allocation failed");
    return -1;
  }

  return 0;
}


/**********************************************************************

    Function    : fsInitialize
    Description : Initialize the file system provided via the file 
    Inputs      : filesys - in-memory file system info
                  fsfile - file containing the state of the file system
    Outputs     : none

***********************************************************************/

void fsInitialize( filesys_t *fs, char *fsfile ) 
{
  int creating = 0;
  int fssize;
  unsigned int block_cache;

  /* set the commands */
  fs->create = createfn;
  fs->open = openfn;
  fs->list = listfn;
  fs->close = closefn;
  fs->read = readfn;
  fs->write = writefn;
  fs->seek = seekfn;
  fs->checkpoint = checkpointfn;

  /* open the fsfile -- contains the file system */
  if (( fs->fd = open( fsfile, O_CREAT | O_RDWR, S_IRWXU )) < 0 ) {
    errorMessage("fsInitialize: cannot open <fsfile>");
    exit( -1 );
  }

  /* get the file size */
  if ( fstat( fs->fd, &fs->sb ) < 0 ) {
    close( fs->fd );
    errorMessage("fsInitialize: cannot stat <fsfile>");
    exit( -1 );
  }

  fssize = fs->sb.st_size;

  /* Initialize the file system information */
  if ( fssize == 0 ) {   /* a new fsfile */

    /* set that we are creating a new file system */
    creating = 1;

    /* compute the size of the file system on disk */
    /* file system struct + (no of blocks * (size of block metadata + size of block data)) */
    fssize = FS_BLOCKS * FS_BLOCKSIZE;

    if ( ftruncate( fs->fd, fssize ) < 0 ) {
      close( fs->fd );
      errorMessage("fsInitialize: cannot ftruncate for a new <fsfile>");
      exit( -1 );
    }
  }

  /* mmap the fsfile */
  if (( fs->base = dfs = (dfilesys_t *)mmap( 0, fssize, PROT_READ | PROT_WRITE, 
					     MAP_SHARED, fs->fd, 0 )) == (void *)-1 ) {
    close( fs->fd );
    errorMessage("fsInitialize: cannot mmap <fsfile>");
    exit( -1 );
  }

  if ( creating ) {    /* create the filesystem */
    dblock_t *blk, *prev = (dblock_t *) NULL; 
    int i;

    dfs->bsize = FS_BLOCKS;                  /* number of blocks total */
    dfs->journal = FS_JOURNAL_SUPER_BLOCK;   /* journal superblock is the second block */

    if ( diskJournalInitialize( (dblock_t *)disk2addr( fs->base, block2offset( dfs->journal ))) < 0 ) {
      errorMessage("fsInitialize: fail in journal initialization");
      exit( -1 );
    }

    dfs->root = FS_SUPER_BLOCKS +   /* root fs, journal super, and journal blocks */
      FS_JOURNAL_BLOCKS;         

    /* make the root directory -- first (only) on-disk dentry block */
    if ( diskDirInitialize( (ddir_t *)disk2addr( fs->base, block2offset( dfs->root ))) < 0 ) {
      errorMessage("fsInitialize: fail in disk directory initialization");
      exit( -1 );
    }

    dfs->firstfree = FS_SUPER_BLOCKS+     /* offset to first free data block */
      FS_JOURNAL_BLOCKS+FS_DIRINIT_BLOCKS;      

    /* initialize the other blocks on disk as free blocks -- link together */
    for ( i = dfs->firstfree; i < FS_BLOCKS; i++ ) {      /* start at first free block */
      blk = (dblock_t *)block2addr( fs->base, i );
      blk->free = FREE_BLOCK;
      blk->st.data_end = 0;
      if ( prev != NULL ) {
	prev->next = (unsigned int)addr2block( blk, fs->base );
      }
      prev = blk;
    }
  }


  /* in-memory file system initialization - file table */
  fs->filetable = (file_t **)malloc( FS_FILETABLE_SIZE * sizeof(file_t *));
  memset( fs->filetable, 0, FS_FILETABLE_SIZE * sizeof(file_t *));

  /* in-memory file system initialization - block cache */
  block_cache = (unsigned int)malloc(( FS_BCACHE_BLOCKS+1 ) * FS_BLOCKSIZE );
  fs->block_cache = addr2blkbase(( block_cache+FS_BLOCKSIZE ));
  memset( fs->block_cache, 0, FS_BCACHE_BLOCKS * FS_BLOCKSIZE );
  memset( fs->d2cmap, BLK_INVALID, sizeof(unsigned int) * FS_BCACHE_BLOCKS ); 
  memset( fs->c2dmap, BLK_INVALID, sizeof(unsigned int) * FS_BCACHE_BLOCKS ); 
  fs->freecache = 0;

  /* in-memory file system initialization - tracking blocks to be journalled per filesys command */
  fsJournalInit( fs );

}


/**********************************************************************

    Function    : processCmd
    Description : run a file system command
    Inputs      : fs - in-memory file system
                  cmdline - command line
    Outputs     : 0 if successful, other represents error or termination

***********************************************************************/

int processCmd( filesys_t *fs, char *cmdline ) 
{
  char *cmd, *arg[MAX_ARGS];
  int i = 0, j;
  char *buf; 
  int index = 0;
 
  fsJournalReset( fs );

  /* parse command and args */
  cmd = strtok( cmdline, cmd_delims );
  while (( cmd != NULL ) && ((arg[i++] = strtok( NULL, arg_delims )) != NULL ));
  
  /* remove whitespace at beginning of each arg */
  for ( j = 0; j < (i-1); j++ ) {
    int k = 0;
    while ( arg[j][k] == ' ' ) k++;
    arg[j] += k;
  }

  /* run commands */
  if (( strcmp( cmd, "creat" ) == 0 ) && ( arg[2] == NULL )) {
    printf("%s: name = %s; mode = %d\n", cmd, arg[0], atoi( arg[1] ));
    index = fs->create( arg[0], atoi( arg[1] ));
    printf("%s: name = %s; fd = %d\n", cmd, arg[0], index );
  }
  else if (( strcmp( cmd, "open" ) == 0 ) && ( arg[2] == NULL )) {
    printf("%s: name = %s; flags = %d\n", cmd, arg[0], atoi( arg[1] ));
    index = fs->open( arg[0], atoi( arg[1] ));
    printf("%s: name = %s; fd = %d\n", cmd, arg[0], index );
  }
  else if (( strcmp( cmd, "list" ) == 0 ) && ( arg[0] == NULL )) {
    printf("%s: void\n", cmd);
    fs->list( );
  }
  else if (( strcmp( cmd, "close" ) == 0 ) && ( arg[1] == NULL )) {
    printf("%s: %d\n", cmd, atoi( arg[0] ));
    fs->close( atoi( arg[0] ));
  }
  else if (( strcmp( cmd, "checkpoint" ) == 0 ) && ( arg[0] == NULL )) {
    printf("%s: checkpoint journal to disk\n", cmd);
    fs->checkpoint( );
  }
  else if (( strcmp( cmd, "read" ) == 0 ) && ( arg[3] == NULL )) {
    buf = (char *)malloc(atoi( arg[2] ) +1);
    memset( buf, 0, atoi( arg[2] ) +1);
    printf("%s: fd = %d; buf = 0x%x; size = %d\n", cmd, atoi( arg[0] ), 
	   (unsigned int)buf, atoi( arg[2] ));
    fs->read( atoi( arg[0] ), buf, atoi( arg[2] ));
    buf[ atoi( arg[2] ) ] = 0;
    printf("\n ======= read: %d; %d bytes ==========\n", atoi( arg[0] ), 
	   atoi( arg[2] ));
    printf("%s\n", buf);
    free( buf );
  }
  else if (( strcmp( cmd, "write" ) == 0 ) && ( arg[3] == NULL )) {
    struct stat sb;
    int file_size, fd;

    printf("%s: fd = %d; buf file = %s; unused = %d\n", cmd, atoi( arg[0] ), 
	   arg[1], atoi( arg[2] ));
    if (( fd = open( arg[1], O_RDONLY )) < 0 ) {
      errorMessage("write command: cannot open <file>");
      return -1;
    }

    if ( fstat( fd, &sb ) < 0 ) {
      close( fd );
      errorMessage("write command: cannot stat <file>");
      return -1;
    }
    
    file_size = sb.st_size;
    buf = (char *) malloc(file_size);

    read( fd, buf, file_size );

    fs->write( atoi( arg[0] ), buf, file_size );
  }
  else if (( strcmp( cmd, "seek" ) == 0 ) && ( arg[2] == NULL )) {
    printf("%s: fd = %d; size = %d\n", cmd, atoi( arg[0] ), 
	   atoi( arg[1] ));
    fs->seek( atoi( arg[0] ), atoi( arg[1] ));
  }
  else if (( strcmp( cmd, "print" ) == 0 ) && ( arg[1] == NULL )) {
    buf = (char *)malloc( 10000 );
    printf("%s: fd = %d", cmd, atoi( arg[0] ));
    fs->seek( atoi( arg[0] ), 0);
    fs->read ( atoi( arg[0] ), buf, 10000);
    printf("\n ======= file contents: %d ==========\n", atoi( arg[0] ));
    printf("%s\n", buf);
    free( buf );
  }
  /* remove file */
  else {
    printf("no match for: %s\n", cmd);
  }

  fsJournalCommit( fs );

  return( 0 );
}


/**********************************************************************

    Function    : runSuite
    Description : run the file system commands 
    Inputs      : fs - in-memory file system
                  cmdfile - file system command file
    Outputs     : none

***********************************************************************/

int runSuite( filesys_t *fs, char *cmdfile ) 
{
  int fd, rc;
  char cmdline[MAX_LINE_SIZE];

  /* open the command file */
  if (( fd = open( cmdfile, O_RDONLY )) < 0 ) {
    errorMessage("runSuite: cannot open <cmdfile>");
    return -1;
  }

  /* read a command line */
  while (( readline( fd, cmdline, MAX_LINE_SIZE )) != 0 ) {

    /* run the command */
    rc = processCmd( fs, cmdline );
    
    if ( rc != 0 )
      break;
  }

  close( fd );

  /* collect stats */
  
  return 0;
}


/**********************************************************************

    Function    : printStats
    Description : output the memory management statistics 
    Inputs      : 
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

void printStats( void )
{
#if 0
  unsigned int freemem = freeMemory( heap->lists );
  unsigned int fragmem = internalFragments( heap->lists );

  printf("\n=================\n");
  printf("Number of operations: %d\n", heap->stats->opct );
  printf("Number of out_of_memory requests: %d\n", heap->stats->outofct );
  printf("Amount of memory allocated: %d bytes\n", heap->stats->bytes );
  printf("Amount of free memory at end: %d bytes\n", freemem );
  printf("Percentage of free memory at end: %d\n", 100*freemem/heap->size );
  printf("Number of allocates: %d\n", heap->stats->allocct );
  printf("Number of frees: %d\n", heap->stats->freect );
  printf("Number of free blocks: %d\n", listSize( heap->holes[0] ));
  printf("Number of internal fragmentation in bytes: %d\n", fragmem );
  printf("Percentage of internal fragmentation: %d\n", 100*fragmem/( heap->size - freemem ));
#endif
  printf("\n");
}


/**********************************************************************

    Function    : main
    Description : this is the main function for project #4
    Inputs      : argc - number of command line parameters
                  argv - the text of the arguments
    Outputs     : 0 if successful, -1 if failure

***********************************************************************/

int main( int argc, char **argv ) 
{
    /* allocate the in-memory file system object */
    fs = (filesys_t *)malloc(sizeof(filesys_t));
  
    /* Check for arguments */
    if ( argc < 3 ) 
    {
        /* Complain, explain, and exit */
        errorMessage( "missing or bad command line arguments\n" );
        printf( CLIENT_USAGE );
        exit( -1 );
    }

    /* setup file system */
    fsInitialize( fs, argv[1] );

    /* create this process */
    processInitialize( );

    /* run the test suite against the memory manager */
    runSuite( fs, argv[2] );

    /* print the statistics */
    printStats( );

    close( fs->fd );

    exit( 0 );
}
