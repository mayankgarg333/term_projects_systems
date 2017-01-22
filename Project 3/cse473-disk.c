
/**********************************************************************

   File          : cse473-disk.c

   Description   : File system function implementations
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

/* Project Include Files */
#include "cse473-filesys.h"
#include "cse473-disk.h"
#include "cse473-list.h"
#include "cse473-util.h"

/* Definitions */

/* program variables */

/* Functions */

    // Approach:
  // find block in cache first - if not get from disk and put in cache
  //    how to find a block in a cache?  need a map
  // when update cached block -> record modified cached blocks somewhere (students choose)
  // just push data blocks to disk
  // write non-data cached blocks recorded to journal (at end of command)
  // when checkpoint -> write journaled blocks to disk blocks

/**********************************************************************

    Function    : diskJournalInitialize
    Description : Initialize the journal on disk
    Inputs      : journal reference
    Outputs     : 0 if success, -1 on error

***********************************************************************/

int diskJournalInitialize( dblock_t *jblk )
{
  journal_t *jrnl = (journal_t *)disk2addr( jblk, sizeof(dblock_t) );

  /* clear journal dblock */
  memset( jblk, 0, FS_BLOCKSIZE );

  /* initialize journal fields */
  jrnl->size = FS_JOURNAL_BLOCKS;      /* number of journal blocks */
  jrnl->start = 0;                     /* first journal block - offset from base - circular queue */
  jrnl->end = 0;                       /* last journal block - offset from base - circular queue*/
  jrnl->ct = 0;                        /* total number of journal blocks used */
  jrnl->next_txn = 0;                  /* index of last transaction run */

  return 0;  
}

/**********************************************************************

    Function    : diskJournalCreateTxnBegin
    Description : Create TXB block for new transaction in journal
    Inputs      : blks - pointer to array of (cached) blocks to be journalled 
                      (store their disk block numbers in TXB)
                  ct - number of blocks to be journalled
    Outputs     : 0 if success, -1 on error

***********************************************************************/

txn_t *diskJournalCreateTxnBegin( jblock_t *blks[], unsigned int ct )
{
  int i = 0;
  dblock_t *jblk = (dblock_t *)disk2addr( fs->base, block2offset( dfs->journal ));
  journal_t *jrnl = (journal_t *)disk2addr( jblk, sizeof( dblock_t ));

  if ( jrnl->ct == FS_JOURNAL_BLOCKS ) {   /* if journal is full - checkpoint */
    diskJournalCheckpoint( );
  }

  /* find journal end and add TXB block at next spot - and increment end */
  unsigned int blknum = ( (jrnl->end++) % FS_JOURNAL_BLOCKS );  /* journal data block index for txb */
  jrnl->ct++;    /* increment journal size */

  dblock_t *txb_blk = (dblock_t *)disk2addr( jbase(fs->base), block2offset( blknum ));
  txn_t *txb = (txn_t *)disk2addr( txb_blk, sizeof( dblock_t ));

  /* initialize txb dblock */
  txb_blk->free = TXB_BLOCK;                          /* TXB begin block */
  txb_blk->st.data_end = BLK_INVALID;                 /* not used */
  txb_blk->next = (blknum + 1) % FS_JOURNAL_BLOCKS;   /* next journal data block index */

  /* populate txb block */
  txb->id = jrnl->next_txn++;   /* increment jrnl transaction id */
  txb->ct = ct;                 /* number of data blocks to be journalled */

  for ( i=0; i < ct; i++ ) {
    txb->blknums[i] = blks[i]->index;  /* record the disk block indices for the journalled blocks here */
  }

  return txb;
}


/**********************************************************************

    Function    : diskJournalCreateBlock
    Description : Log block into journal
    Inputs      : cblk - is a cached block of type dblock_t, but only metadata 
                     blocks are to be logged
    Outputs     : 1 if success, -1 on error

***********************************************************************/

int diskJournalCreateBlock( dblock_t *cblk )
{
  dblock_t *jblk = (dblock_t *)disk2addr( fs->base, block2offset( dfs->journal ));
  journal_t *jrnl = (journal_t *)disk2addr( jblk, sizeof( dblock_t ));

  if ( jrnl->ct == FS_JOURNAL_BLOCKS ) {   /* if journal is full - checkpoint */
    diskJournalCheckpoint( );
  }

  /* find journal end and metadata block at next spot - and increment end/ct */
  unsigned int blknum = (jrnl->end++) % FS_JOURNAL_BLOCKS;  /* journal data block index for txb */
  jrnl->ct++;    /* increment journal size */

  /* copy cached block into journal's block (on-disk) called xblk */
  dblock_t *xblk = (dblock_t *)disk2addr( jbase( fs->base ), block2offset( blknum ));
  memcpy( xblk, cblk, FS_BLOCKSIZE );

  return 1;
}


/**********************************************************************

    Function    : diskJournalCreateTxnEnd
    Description : Create TXE block to complete transaction in journal
    Inputs      : blks - pointer to array of blocks to be journalled (store their disk block numbers in TXB)
                  ct - number of blocks to be journalled
    Outputs     : 0 if success, -1 on error

***********************************************************************/

int diskJournalCreateTxnEnd( txn_t *txb,  unsigned int ct )
{
  dblock_t *jblk = (dblock_t *)disk2addr( fs->base, block2offset( dfs->journal ));
  journal_t *jrnl = (journal_t *)disk2addr( jblk, sizeof( dblock_t ));

  if ( jrnl->ct == FS_JOURNAL_BLOCKS ) {   /* if journal is full - checkpoint */
    diskJournalCheckpoint( );
  }

  // Task #5 
  /* select and populate the TXE block for this transaction */
  // NOTE: Similar to TxnBegin, but not exactly the same 

  unsigned int blknum = ( (jrnl->end++) % FS_JOURNAL_BLOCKS );  /* journal data block index for txe */
  jrnl->ct++;    /* increment journal size */

  dblock_t *txe_blk = (dblock_t *)disk2addr( jbase(fs->base), block2offset( blknum ));
  txn_t *txe = (txn_t *)disk2addr( txe_blk, sizeof( dblock_t ));

  /* initialize txb dblock */
  txe_blk->free = TXE_BLOCK;                          /* TXE begin block */
  txe_blk->st.data_end = BLK_INVALID;                 /* not used */
  txe_blk->next =(blknum + 1) % FS_JOURNAL_BLOCKS;      /* next journal data block index */

  /* populate txb block */
  txe->id = txb->id;   /* increment jrnl transaction id */
  txe->ct = ct;                 /* number of data blocks to be journalled */

  return 1;
}

/**********************************************************************

    Function    : fsJournalCheckpoint
    Description : Apply journalled blocks to the disk
    Inputs      : None
    Outputs     : 0 or error

***********************************************************************/

// Task #6
// write this function

void diskJournalCheckpoint( void )
{
  dblock_t *jblk = (dblock_t *)disk2addr( fs->base, block2offset( dfs->journal ));
  journal_t *jrnl = (journal_t *)disk2addr( jblk, sizeof( dblock_t ));   // journal superblock data

  jrnl->end=jrnl->end%FS_JOURNAL_BLOCKS;

  /* apply transactions from the current start of the journal until end */
  while ( jrnl->start != jrnl->end || jrnl->ct==20 ) {

    // NOTE: for loop to write each block in the journal to the disk block  
    int blknum = 0, jrnl_blk_num = 0;
    dblock_t *jrnl_blk = (dblock_t *)NULL, *dblk = (dblock_t *)NULL; 


     //getting TXB Block
     int txb_blk_num;
     txb_blk_num= dfs->journal+jrnl->start+1;
     dblock_t *txb_block=(dblock_t *)disk2addr( fs->base, block2offset(txb_blk_num));
     txn_t *txb = (txn_t *)disk2addr( txb_block, sizeof( dblock_t ));
     // getting TXE block
     int txe_blk_num;
     txe_blk_num= 2+ (dfs->journal+jrnl->start+txb->ct)%(FS_JOURNAL_BLOCKS);
     dblock_t *txe_block=(dblock_t *)disk2addr( fs->base, block2offset(txe_blk_num)); // check the rap around
     if (txe_block->free!=TXE_BLOCK)
          break; //If no TXE block
    // txn_t *txe = (txn_t *)disk2addr( txe_block, sizeof( dblock_t ));


     // loop through TXB to TXE
     int i=0;
     for(i=0;i<txb->ct;i++){
     	
	jrnl_blk_num= 2+ (dfs->journal+jrnl->start+i)%(FS_JOURNAL_BLOCKS);
     	jrnl_blk=(dblock_t *)disk2addr( fs->base, block2offset(jrnl_blk_num));
	
	dblk=(dblock_t *) disk2addr(fs->base,block2offset(txb->blknums[i]));
     	blknum=txb->blknums[i];



    // journal: jrnl_blk - dblock_t for the journal block
    // jrnl_blk_num - block number on disk of journal block (should be 2-21)
    printf("journal: jblk #: %d; jblk->free: %d; jblk->st: %d; jblk->next: %d\n", 
	   jrnl_blk_num, jrnl_blk->free, jrnl_blk->st.data_end, jrnl_blk->next );
    // before: dblk - disk block info before update using journal block 
    // blknum - block number on disk of the block (> 23)
    printf("before: dblk #: %d; dblk->free: %d; dblk->st: %d; dblk->next: %d\n", 
	   blknum, dblk->free, dblk->st.data_end, dblk->next );
    // checkpt: dblk - disk block info before update using journal block
    
    memcpy(dblk,jrnl_blk,FS_BLOCKSIZE);
    printf("checkpt: dblk #: %d; dblk->free: %d; dblk->st: %d; dblk->next: %d\n", 
	   blknum, dblk->free, dblk->st.data_end, dblk->next );
    
    jrnl_blk->free=FREE_BLOCK;
    }
    // Task #7
    // Set the blocks used in the journal transaction to free (FREE_BLOCK) and update the
    // journal (jrnl) metadata (start, ct, end, ...) as necessary  
    
    txb_block->free=FREE_BLOCK;
    txe_block->free=FREE_BLOCK;

    jrnl->start=(jrnl->start+txb->ct+2)%FS_JOURNAL_BLOCKS;
    jrnl->ct=jrnl->ct - txb->ct -2;

  }
   
 
}


/**********************************************************************

    Function    : blockFindCached
    Description : Retrieve block from block cache
    Inputs      : blknum - block index on the disk
    Outputs     : block or NULL

***********************************************************************/

dblock_t *blockFindCached( unsigned int blknum ) 
{
  if ( blknum != BLK_INVALID ) {
    unsigned int cache_blknum = fs->d2cmap[blknum];
    
    if ( cache_blknum != BLK_INVALID ) {
      return (dblock_t *)disk2addr( fs->block_cache, block2offset( cache_blknum ));
    }
  }
  return (dblock_t *)NULL;
}


/**********************************************************************

    Function    : blockAddCached
    Description : Add disk block to the block cache
    Inputs      : dblk - disk block
                  blknum - block index on the disk
    Outputs     : block or NULL

***********************************************************************/

dblock_t *blockAddCached( dblock_t *dblk, unsigned int blknum ) 
{
  int cache_blknum = fs->freecache++;
  dblock_t *cblk;

  assert( cache_blknum < FS_BCACHE_BLOCKS );
  assert( dblk != NULL ); 
  assert( blknum != BLK_INVALID );   // there is a real dblk

  /* if not cached, create cache block and update map */ 
  cblk = (dblock_t *)disk2addr( fs->block_cache, block2offset( cache_blknum ));
  fs->d2cmap[blknum] = cache_blknum;
  fs->c2dmap[cache_blknum] = blknum;

  /* copy dblk into cblk */
  memcpy( cblk, dblk, FS_BLOCKSIZE );

  return cblk;
}


/**********************************************************************

    Function    : blockToBeJournalled
    Description : Add disk block to a list for recording in the journal
    Inputs      : blk - block that will be journalled when the command completes
                  blknum - block index on the disk
    Outputs     : None

***********************************************************************/

// Task #2

void blockToBeJournalled( dblock_t *cblk, unsigned int blknum )
{
  /* add this block and associated block number (for disk block) to the 
     file system journal (fs->journal_blks) */
   fs->journal_blks[fs->journal_blk_count]->blk=cblk;
   fs->journal_blks[fs->journal_blk_count++]->index=blknum;
}


/**********************************************************************

    Function    : diskDirInitialize
    Description : Initialize the root directory on disk
                  NOTE: Not using dblock header in this block!  
                  Works because this block is at a fixed location
                  but may want to make consistent
    Inputs      : directory reference
    Outputs     : 0 if success, -1 on error

***********************************************************************/

int diskDirInitialize( ddir_t *ddir )
{
  /* Local variables */
  dblock_t *first_dentry_block;
  int i;
  ddh_t *ddh; 

  /* clear disk directory object */
  memset( ddir, 0, FS_BLOCKSIZE );

  /* initialize disk directory fields */
  ddir->buckets = ( FS_BLOCKSIZE - sizeof(ddir_t) ) / (sizeof(ddh_t));
  ddir->freeblk = FS_SUPER_BLOCKS+    /* super blocks + journal blocks + this directory block */
    FS_JOURNAL_BLOCKS+1;           
  ddir->free = 0;                 /* dentry offset in that block */

  /* assign first dentry block - for directory itself */
  first_dentry_block = (dblock_t *)disk2addr( fs->base, block2offset( ddir->freeblk ));
  memset( first_dentry_block, 0, FS_BLOCKSIZE );
  first_dentry_block->free = DENTRY_BLOCK;
  first_dentry_block->st.dentry_map = DENTRY_MAP;
  first_dentry_block->next = BLK_INVALID;

  /* initialize ddir hash table */
  ddh = (ddh_t *)ddir->data;     /* start of hash table data -- in ddh_t's */
  for ( i = 0; i < ddir->buckets; i++ ) {
    (ddh+i)->next_dentry = BLK_SHORT_INVALID;
    (ddh+i)->next_slot = BLK_SHORT_INVALID;
  }

  return 0;  
}


/**********************************************************************

    Function    : diskReadDir
    Description : Retrieve the on-disk directory -- only one in this case
    Inputs      : name - name of the file 
    Outputs     : on-disk directory or -1 if error

***********************************************************************/

ddir_t *diskReadDir( char *name ) 
{
  return ((ddir_t *)disk2addr( fs->base, block2offset( dfs->root )));
}


/**********************************************************************

    Function    : diskFindDentry
    Description : Retrieve the on-disk dentry from the disk directory
    Inputs      : diskdir - on-disk directory
                  name - name of the file 
    Outputs     : on-disk dentry or NULL if error

***********************************************************************/


ddentry_t *diskFindDentry( ddir_t *diskdir, char *name ) 
{
  int key = fsMakeKey( name, diskdir->buckets );
  ddh_t *ddh = (ddh_t *)&diskdir->data[key];

  while (( ddh->next_dentry != BLK_SHORT_INVALID ) || ( ddh->next_slot != BLK_SHORT_INVALID )) {
    dblock_t *cblk;

    // find block to examine
    // see if in block cache - if not copy into block cache and continue
    // then do work as before...

    if ( (cblk = blockFindCached( ddh->next_dentry )) == (dblock_t *)NULL ) {
      dblock_t *dblk = (dblock_t *)disk2addr( fs->base, block2offset( ddh->next_dentry ));
      cblk = blockAddCached( dblk, ddh->next_dentry );
    }

    ddentry_t *disk_dentry = (ddentry_t *)disk2addr( cblk, dentry2offset( ddh->next_slot ));
    
    if ( strcmp( disk_dentry->name, name ) == 0 ) {
      return disk_dentry;
    }

    ddh = &disk_dentry->next;
  }

  return (ddentry_t *)NULL;  
}


/**********************************************************************

    Function    : diskFindFile
    Description : Retrieve the on-disk file from the on-disk dentry
    Inputs      : disk_dentry - on-disk dentry
    Outputs     : on-disk file control block or NULL if error

***********************************************************************/

fcb_t *diskFindFile( ddentry_t *disk_dentry ) 
{
  dblock_t *cblk;

  if ( disk_dentry->block == BLK_INVALID ) {
    errorMessage("diskFindFile: no such file");
    printf("\nfile name = %s\n", disk_dentry->name);
    return (fcb_t *)NULL;  
  }

  // find block to examine
  // see if in block cache - if not copy into block cache and continue
  // then do work as before...

  if ( (cblk = blockFindCached( disk_dentry->block )) == (dblock_t *)NULL ) {
      dblock_t *dblk = (dblock_t *)disk2addr( fs->base, block2offset( disk_dentry->block ));
      cblk = blockAddCached( dblk, disk_dentry->block );
  }

  return (fcb_t *)disk2addr( cblk, sizeof(dblock_t) );
}


/**********************************************************************

    Function    : diskCreateDentry
    Description : Create disk entry for the dentry on directory
    Inputs      : base - ptr to base of file system on disk
                  dir - in-memory directory
                  dentry - in-memory dentry
    Outputs     : none

***********************************************************************/

void diskCreateDentry( unsigned int base, dir_t *dir, dentry_t *dentry ) 
{
  ddir_t *diskdir = dir->diskdir;
  ddentry_t *disk_dentry;
  dblock_t *dblk, *cblk, *nextblk = (dblock_t *) NULL, *next_dblk;
  ddh_t *ddh;
  unsigned int cblknum = BLK_INVALID;
  unsigned int nextblknum = BLK_INVALID;
  int empty = 0;
  int key;

  // create buffer cache for blocks retrieved from disk - not mmapped
  // journal copies of buffer cache 
  //   and write to mmapped journal
  // then restart and write journal to the filesystem

  /* find location for new on-disk dentry */
  cblknum = diskdir->freeblk;
  if ( (cblk = blockFindCached( cblknum )) == (dblock_t *)NULL ) {
    dblk = (dblock_t *)disk2addr( base, (block2offset( diskdir->freeblk )));
    cblk = blockAddCached( dblk, cblknum );
  }

  disk_dentry = (ddentry_t *)disk2addr( cblk, dentry2offset( diskdir->free ));

  /* associate dentry with ddentry */
  dentry->diskdentry = disk_dentry;  // P3 - disk_dentry is actually a cached block
  
  // P3 - metadata - disk_dentry
  /* update disk dentry with dentry's data */
  strcpy( disk_dentry->name, dentry->name );
  disk_dentry->block = BLK_INVALID;

  /* push disk dentry into on-disk hashtable */
  key = fsMakeKey( disk_dentry->name, diskdir->buckets );
  ddh = diskDirBucket( diskdir, key );
  /* at diskdir's hashtable bucket "key", make this disk_dentry the next head
     and link to the previous head */
  disk_dentry->next.next_dentry = ddh->next_dentry;   // P3
  disk_dentry->next.next_slot = ddh->next_slot;       // P3
  ddh->next_dentry = diskdir->freeblk;
  ddh->next_slot = diskdir->free;

  /* set this disk_dentry as no longer free in the block */
  clearbit( cblk->st.dentry_map, diskdir->free, DENTRY_MAX );   // P3 - seem to need a copy in fs

  /* update free reference for dir */
  /* first the block, if all dentry space has been consumed */
  if ( cblk->st.dentry_map == 0 ) { /* no more space for dentries here */
    /* need another directory block for disk dentries */
    if ( cblk->next == BLK_INVALID ) {       /* get next file system free block */
      diskdir->freeblk = dfs->firstfree;
      
      /* update file system's free blocks */
      /* NOTE: fs not journaled yet */
      nextblknum = dfs->firstfree;
      if ( (nextblk = blockFindCached( nextblknum )) == (dblock_t *)NULL ) {
	next_dblk = (dblock_t *)disk2addr( base, (block2offset( nextblknum )));
	nextblk = blockAddCached( next_dblk, nextblknum );
      }

      dfs->firstfree = nextblk->next;
      nextblk->free = DENTRY_BLOCK;   /* this is now a dentry block */
      nextblk->st.dentry_map = DENTRY_MAP;
      nextblk->next = BLK_INVALID;
    }
    else { /* otherwise use the next dentry block */
      diskdir->freeblk = cblk->next;      
    }
  }

  /* now update the free entry slot in the block */
  /* find the empty dentry slot */
  empty = findbit( cblk->st.dentry_map, DENTRY_MAX );
  diskdir->free = empty;
  
  // Task #3 
  // Which blocks to journal?  call blockToBeJournalled

  blockToBeJournalled(cblk,cblknum);
  
  if(nextblk){
  	blockToBeJournalled(nextblk,nextblknum);
  }

  if (empty == BLK_INVALID ) {
    errorMessage("diskCreateDentry: bad bitmap");
    return;
  }      
}


/**********************************************************************

    Function    : diskCreateFile
    Description : Create file block for the new file
    Inputs      : base - ptr to base of file system on disk
                  dentry - in-memory dentry
                  file - in-memory file
    Outputs     : 0 on success, <0 on error 

***********************************************************************/

// TJ - create block cache version of fcb and dentry
// Regular data blocks may be left to the disk - allocDblocks will need to know tho

int diskCreateFile( unsigned int base, dentry_t *dentry, file_t *file )
{
  dblock_t *fblk, *cblk;
  fcb_t *fcb;
  ddentry_t *disk_dentry;
  int i;
  unsigned int block;

  // P3 - allocate block # on disk
  allocDblock( &block, FILE_BLOCK );   

  if ( block == BLK_INVALID ) {
    return -1;
  }  

  /* find a file block in file system */
  // P3 - get block cache version of disk block
  // then metadata updates are easy

  if ( (cblk = (dblock_t *)blockFindCached( block )) == (dblock_t *)NULL ) {
      fblk = (dblock_t *)disk2addr( base, (block2offset( block )));
      cblk = (dblock_t *)blockAddCached( fblk, block );
  }

  fcb = (fcb_t *)disk2addr( cblk, sizeof( dblock_t ));   /* file is offset from block info */

  /* associate file with the cached on-disk file */
  file->diskfile = fcb;

  // P3 - metadata 
  /* set file data into file block */
  fcb->flags = file->flags;
  /* initialize attributes - XXX: not used in this project */
  fcb->attr_block = BLK_INVALID;    /* no block yet */

  /* initial on-disk block information for file */  
  for ( i = 0; i < FILE_BLOCKS; i++ ) {
    fcb->blocks[i] = BLK_INVALID;   /* initialize to empty */
  }

  /* get on-disk dentry */
  // P3 - store block cache diskdentry? and update and journal that
  disk_dentry = dentry->diskdentry;

  /* set file block in on-disk dentry */
  disk_dentry->block = block;

  // Task #3 
  // Which blocks to journal?  call blockToBeJournalled

  blockToBeJournalled(cblk,block);
  
  blockToBeJournalled(addr2blkbase(disk_dentry),caddr2dblk( disk_dentry, fs->block_cache, fs->c2dmap ));
  
  return 0;
}


/**********************************************************************

    Function    : diskWrite
    Description : Write the buffer to the disk
    Inputs      : disk_offset - pointer to place where offset is stored on disk
                  block - index to block to be written
                  buf - data to be written
                  bytes - the number of bytes to write
                  offset - offset from start of file
                  sofar - bytes written so far
    Outputs     : number of bytes written or -1 on error 

***********************************************************************/

unsigned int diskWrite( unsigned int *disk_offset, unsigned int block, 
			char *buf, unsigned int bytes, 
			unsigned int offset, unsigned int sofar )
{
  dblock_t *dblk, *cblk;
  char *start, *end, *data;
  int block_bytes;
  unsigned int blk_offset = offset % FS_BLKDATA;

  /* get the cache blocks - write to cached first */
  dblk = (dblock_t *)disk2addr( fs->base, (block2offset( block )));
  if ( (cblk = (dblock_t *)blockFindCached( block )) == (dblock_t *)NULL ) {
    cblk = (dblock_t *)blockAddCached( dblk, block );
  }

  /* compute the block addresses and range */
  data = (char *)disk2addr( cblk, sizeof(dblock_t) );
  start = (char *)disk2addr( data, blk_offset );
  //  end = (char *)disk2addr( fs->base, (block2offset( (block+1) )));
  end = (char *)disk2addr( cblk, FS_BLOCKSIZE );
  block_bytes = min(( end - start ), ( bytes - sofar ));

  /* do the write to cache block */
  memcpy( start, buf, block_bytes );
  
  /* compute new offset, and update in fcb if end is extended */
  offset += block_bytes;
  
  if ( offset > *disk_offset ) {
    *disk_offset = offset;
  }

  // Task #1
  /* write the cached block to the journal or disk?  */
  memcpy(dblk,cblk,FS_BLOCKSIZE); 
  
  return block_bytes;  
}


/**********************************************************************

    Function    : diskRead
    Description : read the buffer from the disk
    Inputs      : block - index to file block to read
                  buf - buffer for data
                  bytes - the number of bytes to read
                  offset - offset from start of file
                  sofar - bytes read so far 
    Outputs     : number of bytes read or -1 on error 

***********************************************************************/

unsigned int diskRead( unsigned int block, char *buf, unsigned int bytes, 
		       unsigned int offset, unsigned int sofar )
{
  dblock_t *dblk, *cblk;
  char *start, *end, *data;
  int block_bytes;
  unsigned int blk_offset = offset % FS_BLKDATA;

  if ( (cblk = (dblock_t *)blockFindCached( block )) == (dblock_t *)NULL ) {
    dblk = (dblock_t *)disk2addr( fs->base, (block2offset( block )));
    cblk = (dblock_t *)blockAddCached( dblk, block );
  }
  /* compute the block addresses and range */
  data = (char *)disk2addr( cblk, sizeof(dblock_t) );
  start = (char *)disk2addr( data, blk_offset );
  // end = (char *)disk2addr( fs->base, (block2offset( (block+1) )));
  end = (char *)disk2addr( cblk, FS_BLOCKSIZE );
  block_bytes = min(( end - start ), ( bytes - sofar ));

  /* do the read */
  memcpy( buf, start, block_bytes );

  return block_bytes;  
}


/**********************************************************************

    Function    : diskGetBlock
    Description : Get the block corresponding to this file location
    Inputs      : file - in-memory file pointer
                  index - block index in file
    Outputs     : block index or BLK_INVALID

***********************************************************************/

unsigned int diskGetBlock( file_t *file, unsigned int index )
{
  fcb_t *fcb = file->diskfile;
  unsigned int dblk_index;

  if ( fcb == NULL ) {
    errorMessage("diskGetBlock: No file control block for file");
    return BLK_INVALID;
  }

  /* if the index is already in the file control block, then return that */
  dblk_index = fcb->blocks[index]; 
 
  if ( dblk_index != BLK_INVALID ) {
    return dblk_index;
  }

  allocDblock( &dblk_index, FILE_DATA );

  if ( dblk_index == BLK_INVALID ) {
    return BLK_INVALID;
  }

  // P3: Meta-Data 
  /* update the cached fcb with the new block */
  fcb->blocks[index] = dblk_index;

  // Task #3 
  // Which blocks to journal?  call blockToBeJournalled
  // NOTE: See caddr2dblk to compute block number (in cache) from cached fcb

  blockToBeJournalled(addr2blkbase(fcb),caddr2dblk(fcb,fs->block_cache,fs->c2dmap));

  return dblk_index;
}


/**********************************************************************

    Function    : allocDblock
    Description : Get a free data block
    Inputs      : index - index for the block found or BLK_INVALID (set to block)
                  blk_type - the type of use for the block
    Outputs     : 0 on success, <0 on error                  

***********************************************************************/

int allocDblock( unsigned int *index, unsigned int blk_type ) 
{
  dblock_t *dblk;
  dblock_t *cblk;

  /* if there is no free block, just return */
  if ( dfs->firstfree == BLK_INVALID ) {
    *index = BLK_INVALID;
    return BLK_INVALID;
  }

  /* get from file system's free list */
  *index = dfs->firstfree;

  /* get the filesystem block off disk */
  dblk = (dblock_t *)disk2addr( fs->base, (block2offset( *index )));

  /* initialize block cache version of new block */
  cblk = blockAddCached( dblk, *index );
  
  /* mark block as a file block */
  // P3 - metadata update below
  cblk->free = blk_type;

  /* update next freeblock in file system */
  dfs->firstfree = cblk->next;

  return 0;
}
    
