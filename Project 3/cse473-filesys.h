/**********************************************************************

   File          : cse473-filesys.h

   Description   : This file contains the file system prototypes

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

/* defines */
#define TRUE 1

/* file system in general */
#define FS_BLOCKS 70
#define FS_SUPER_BLOCKS 2            // 2 filesystem super blocks: fs super block, journal super block
#define FS_FILESYS_SUPER_BLOCK 0     // block number of filesystem superblock 
#define FS_JOURNAL_SUPER_BLOCK 1     // block number of journal superblock
#define FS_JOURNAL_BLOCKS 20         // blocks allocated for journalling
#define FS_DIRINIT_BLOCKS 2          // dir block and first dentry block are initialized after journal
#define FS_BLOCKSIZE 512             // size of each block
#define MAX_NAME_SIZE 38             // maximum name size for filename (DOS)
#define BLK_INVALID  0xFFFFFFFF      // identifier that a block is not in use
#define BLK_SHORT_INVALID  0xFFFF    // 2-byte version of indentifier
#define FS_BLOCK_MASK 0xFFFFFE00     // 512 byte aligned addresses - need to align cache

/* disk defines */
#define DENTRY_MAP 0x3FF  /* 10 entries per block */
#define DENTRY_MAX 10     /* 10 entries per block */

/* block free values - in "free" field of block - identifies free or type of block if non-zero */
#define FREE_BLOCK 0                 // block is available
#define DENTRY_BLOCK 1               // ddentry_t block (directory entry)
#define FILE_BLOCK 2                 // fcb_t block (file control block)
#define FILE_DATA 3                  // plain data block containing file data
#define FS_BLOCK 4                   // dfilesys_t block (filesystem superblock)
#define DIR_BLOCK 5                  // ddir_t block (directory block)
#define ATTR_BLOCK 6                 // N/A 
#define JOURNAL_BLOCK 7              // journal_t block (journal superblock)
#define TXB_BLOCK 8                  // txn_t block (transaction begin)
#define TXE_BLOCK 9                  // txn_t block (transaction end)

/* file system defines */
#define FS_FILETABLE_SIZE 100    // overall filetable size
#define PROC_FILETABLE_SIZE 20   // per-process filetable size
#define FS_BCACHE_BLOCKS 50      // block cache blocks available
#define FILE_BLOCKS 10           // data blocks per file max
#define XATTR_BLOCKS 5

/* extended attribute defines - P3: not used */
#define XATTR_CREATE  1
#define XATTR_REPLACE 2

/* xattr block create defines - P3: not used */
#define BLOCK_CREATE  1
#define BLOCK_PRESENT 2

/* macros */

/* translate between disk location and memory location */
#define disk2addr( mbase, doffset ) ( ((char *)mbase)+doffset )
#define addr2disk( mptr, mbase ) ( ((char *)mptr)-mbase )
#define block2addr( mbase, block_num ) ( ((char *) mbase ) + ( FS_BLOCKSIZE * block_num ))
#define addr2block( mptr, mbase ) ( (((char *) mptr ) - ((char *) mbase)) / FS_BLOCKSIZE )
#define caddr2dblk( cptr, cbase, map ) ( map[addr2block( cptr, cbase )] )
#define addr2blkbase( mptr ) ( (void *)((unsigned int)mptr & FS_BLOCK_MASK ))
#define jbase( fsbase ) ( fsbase + FS_SUPER_BLOCKS * FS_BLOCKSIZE )

static inline unsigned int fsMakeKey( char *name, int limit ) 
{
  int len = strlen(name);
  int i, val = 1;

  for ( i = 0; i < len; i++ ) {
    val = ( (int)name[i] * val ) % limit;
  }
  
  return val;
}

/**********************************************************************

    Structure    : journal_t 
    Purpose      : Journal superblock - stores its size and current start 
                   and end block indices for the journalled data.  Blocks 
                   are journalled in a circular queue

***********************************************************************/

typedef struct journal {
  unsigned int size;      /* size in blocks */
  unsigned int start;     /* current first block in journal */
  unsigned int end;       /* current last block in the journal (inclusive) */
  unsigned int ct;        /* total number of journal blocks used so far */
  unsigned int next_txn;  /* last transaction index */
} journal_t;


/**********************************************************************

    Structure    : dblock_t 
    Purpose      : corresponds to actual block on the disk -- includes
                   whether the block is free (a next block index if it 
                   is free), a dentry map (for free dentries slots in 
                   a dentry block) and data (e.g., file data, but 
                   means general block data

***********************************************************************/

/* data structure for the blocks on disk */
typedef struct dblock {
  unsigned int free;         /* is the block free? also the type of data stored  */
  union status {
    unsigned int dentry_map; /* free dentries bitmap -- out of 10 per block */
    unsigned int data_end;   /* end pointer of file data in block */
  } st;
  unsigned int next;         /* next disk block index -- in free list */
  char data[0];              /* the rest of the block is data -- depends on block type (free) */
} dblock_t;

typedef char block_t;        /* block is a character array of size FS_BLOCKSIZE */


/**********************************************************************

    Structure    : jblock_t 
    Purpose      : A disk block to be journalled and its associated disk
                   block index

***********************************************************************/

typedef struct jblock {
  dblock_t *blk;           /* block to be journalled */
  unsigned int index;      /* disk block number */
} jblock_t;


/**********************************************************************

    Structure    : txn_t 
    Purpose      : this is the transaction block format for the journal

***********************************************************************/

/* on-disk data structure for journal transaction (start and end) */
typedef struct transaction {
  unsigned int id;          /* transaction identifier */
  unsigned int ct;          /* number of data blocks in transaction (excludes begin and end) */
  unsigned int blknums[0];  /* indices to disk blocks for each journalled data block */
} txn_t;


/**********************************************************************

    Structure    : fcb_t 
    Purpose      : this is the file control block -- persistent storage
                   for file attributes and the file data blocks 
                   (actual number of data blocks max determined by FILE_BLOCKS)

***********************************************************************/

/* on-disk data structure for a file -- file control block */
typedef struct file_control_block {
  unsigned int flags;      /* file flags */
  unsigned int size;       /* size of the file on disk */
  unsigned int attr_block; /* index to first attribute block - NOT USED */
  unsigned int blocks[0];  /* indices to data blocks for rest of file block */
} fcb_t;


/**********************************************************************

    Structure    : file_t 
    Purpose      : this corresponds to an inode -- the in-memory 
                   representation for the file (system-wide).  Includes
                   attributes, including name (names are stored
                   with dentries on the disk -- not fcb), reference count, 
                   reference to the fcb location (in ram-disk), and 
                   a set of file blocks

***********************************************************************/

/* in-memory data structure for a file (inode) */
typedef struct file {
  char name[MAX_NAME_SIZE];          /* file name */
  unsigned int flags;                /* file flags */
  unsigned int size;                 /* file size */
  unsigned int ct;                   /* reference count */
  fcb_t *diskfile;                   /* fcb pointer in ramdisk */
  unsigned int attr_block;           /* index to first attribute block - NOT USED */
  unsigned int blocks[FILE_BLOCKS];  /* direct blocks */
} file_t;


/**********************************************************************

    Structure    : fstat_t 
    Purpose      : this corresponds to the per-process file structure.
                   Mainly care about the current offset for the file
                   for determining where to read or write

***********************************************************************/

/* in-memory data structure for a file operation status */
typedef struct fstat {
  file_t *file;          /* pointer to system-wide in-memory file reference */
  unsigned int offset;   /* current offset index for reads/writes/seeks */
} fstat_t;


/**********************************************************************

    Structure    : ddh_t 
    Purpose      : represents a location for a dentry on the disk -- 
                   which block and which slot in the block -- used 
                   for dentry hash table entries (linked list)

***********************************************************************/

/* disk directory entry hash component */
typedef struct ddentry_hash {
  unsigned short next_dentry; /* next dentry block in hash table */
  unsigned short next_slot;   /* next dentry slot in hash table */
} ddh_t;


/**********************************************************************

    Structure    : ddentry_t  
    Purpose      : represents an on-disk directory entry -- includes
                   the file name and index for the first block in the file
                   (also reference to next dentry for hash table's list)

***********************************************************************/

/* disk directory entry on disk */
typedef struct ddentry {
  char name[MAX_NAME_SIZE];   /* name of file */
  unsigned int block;         /* block number of first block of file */
  ddh_t next;                 /* next dentry in hash table */
} ddentry_t;


/**********************************************************************

    Structure    : dentry_t  
    Purpose      : represents an in-memory directory entry -- includes
                   the file name, file pointer, on-disk dentry memory 
                   location, and next entry in the in-memory hash table

***********************************************************************/

/* in-memory directory entry stores a reference to a file in a directory */
typedef struct dentry {
  file_t *file;              /* file reference */
  char name[MAX_NAME_SIZE];  /* file name */
  ddentry_t *diskdentry;     /* reference to corresponding on-disk structure */
  struct dentry *next;       /* next dentry in the list */
} dentry_t;


/**********************************************************************

    Structure    : ddir_t  
    Purpose      : represents an on-disk directory -- each directory 
                   stores a hash table of on-disk directory entries 
                   (only the first is stored in this block -- the rest 
                    are obtained from ddh_t info), a reference to the 
                    first free spot for a new ddentry (freeblk for block
                    and free for free dentry slot)
                   data stores the hash table buckets

***********************************************************************/

/* disk data structure for the directory */
/* directory stores a hash table to find files */
typedef struct ddir {
  unsigned int buckets;        /* number of hash table buckets in the directory */
  unsigned int freeblk;        /* number of the next free block for dentry */
  unsigned int free;           /* first free dentry */
  ddh_t data[0];               /* reference to the hash table */
} ddir_t;


/**********************************************************************

    Structure    : dir_t  
    Purpose      : represents an in-memory directory -- each directory 
                   stores a hash table of in-memory directory entries
                   (normal in-memory hash table) and reference to 
                   disk directory in ramdisk

***********************************************************************/

typedef struct dir {
  unsigned int bucket_size; /* number of buckets in the dentry hash table */
  dentry_t **buckets;        /* hash table for directory entries */
  ddir_t *diskdir;          /* reference to the directory in ramdisk */
} dir_t;


/**********************************************************************

    Structure    : proc_t
    Purpose      : represents the in memory process -- just to store 
                   a reference the per-process file table

***********************************************************************/

/* in-memory per-process file table */
typedef struct process {
  fstat_t **fstat_table;     /* per-process open file table */
} proc_t;


/**********************************************************************

    Structure    : dfilesys
    Purpose      : represents the on-disk file system -- stores the 
                   overall file system information: number of blocks, 
                   the firstfree block on disk, and the block for the 
                   root directory

***********************************************************************/

/* data structure for a file system */
typedef struct dfilesys {
  unsigned int bsize;     /* number of blocks in the file system */
  unsigned int journal;   /* offset in blocks to the journal superblock */ 
  unsigned int firstfree; /* offset in blocks to the first free block */ 
  unsigned int root;      /* offset to the root directory block */
} dfilesys_t;


/**********************************************************************

    Structure    : filesys
    Purpose      : represents the in-memory file system (superblock) 
                   and ad hoc info about bootstrapping the ramdisk --
                   fd refs to the file descriptor for the ramdisk file,
                   base is the base memory address of the ramdisk, 
                   sb is file stat for the ramdisk file,
                   dir is the in-memory root directory, 
                   filetable is the system-wide, in-memory file table, 
                   proc is our process (yes only one at a time, please)
                   function pointers for the file system commands

***********************************************************************/

/* in-memory info for the file system */
typedef struct filesys {
  int fd;                 /* mmap'd fs file descriptor */
  void *base;             /* mmap pointer -- start of mmapped region */
  struct stat sb;         /* stat buffer for mmapped file */
  dir_t *dir;             /* root directory in the file system (only directory) */
  file_t **filetable;     /* system-wide open file table */
  block_t **block_cache;  /* cache of blocks read into memory */
  unsigned int d2cmap[FS_BCACHE_BLOCKS];  /* map of disk blocks to cache blocks */
  unsigned int c2dmap[FS_BCACHE_BLOCKS];  /* map of cache blocks to disk blocks */
  unsigned int freecache; /* index of free block in cache */
  proc_t *proc;           /* process making requests to the fs */
  jblock_t *journal_blks[FS_JOURNAL_BLOCKS]; /* blocks to be journalled */
  unsigned int journal_blk_count; 
  int (*create)( char *name, unsigned int flags );      /* file create */
  int (*open)( char *name, unsigned int flags );        /* file open */
  void (*list)( void );                                 /* list directory */
  void (*close)( unsigned int fd );                     /* close file descriptor */
  int (*read)( unsigned int fd, char *buf, unsigned int bytes );       /* file read */
  int (*write)( unsigned int fd, char *buf, unsigned int bytes );      /* file write */
  int (*seek)( unsigned int fd, unsigned int index );    /* file seek */
  int (*checkpoint)( void );                            /* perform filesystem checkpoint */
} filesys_t;


/* Global variables */
filesys_t *fs;             /* memory filesystem object */
dfilesys_t *dfs;           /* on-disk filesystem object */

/**********************************************************************

    Function    : fsJournalInit
    Description : alloc memory for collecting blocks for journalling and reset
    Inputs      : fs - filesystem
    Outputs     : None

***********************************************************************/

extern void fsJournalInit( filesys_t *fs );


/**********************************************************************

    Function    : fsJournalReset
    Description : Reset filesystem's cache of blocks to be journalled
    Inputs      : fs - filesystem
    Outputs     : None

***********************************************************************/

extern void fsJournalReset( filesys_t *fs );


/**********************************************************************

    Function    : fsJournalCommit
    Description : Apply blocks marked for journalling to the journal
    Inputs      : fs - filesystem
    Outputs     : Count of size of journal update

***********************************************************************/

extern int fsJournalCommit( filesys_t *fs );


/**********************************************************************

    Function    : fsJournalCheckpoint
    Description : Apply journalled blocks to the disk
    Inputs      : None
    Outputs     : 0 or error

***********************************************************************/

extern int fsJournalCheckpoint( void );

/**********************************************************************

    Function    : fsReadDir
    Description : Return directory entry for name
    Inputs      : name - name of the file 
    Outputs     : new in-memory directory or NULL

***********************************************************************/

extern dir_t *fsReadDir( char *name );

/**********************************************************************

    Function    : fsFileInitialize
    Description : Create a memory file for the specified file
    Inputs      : dir - directory object
                  name - name of the file 
                  flags - flags for file access
                  fcb - file control block (on-disk) reference for file (optional)
    Outputs     : new file or NULL

***********************************************************************/

extern file_t *fsFileInitialize( dentry_t *dentry, char *name, unsigned int flags, fcb_t *fcb );

/**********************************************************************

    Function    : fsDentryInitialize
    Description : Create a memory dentry for this file
    Inputs      : name - file name 
                  disk_dentry - on-disk dentry object (optional)
    Outputs     : new directory entry or NULL

***********************************************************************/

extern dentry_t *fsDentryInitialize( char *name, ddentry_t *disk_dentry );

/**********************************************************************

    Function    : fsAddDentry
    Description : Add the dentry to its directory
    Inputs      : dir - directory object
                  dentry - dentry object
    Outputs     : 0 if success, -1 if error 

***********************************************************************/

extern int fsAddDentry( dir_t *dir, dentry_t *dentry );

/**********************************************************************

    Function    : fsAddFile
    Description : Add the file to the system-wide open file cache
    Inputs      : filetable - system-wide file table 
                  file - file to be added
    Outputs     : a file descriptor, or -1 on error 

***********************************************************************/

extern int fsAddFile( file_t **filetable, file_t *file);

/**********************************************************************

    Function    : fileCreate
    Description : Create directory entry and file object
    Inputs      : name - name of the file 
                  flags - creation options
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

extern int fileCreate( char *name, unsigned int flags );

/**********************************************************************

    Function    : fileOpen
    Description : Open directory entry of specified name
    Inputs      : name - name of the file 
                  flags - creation options
    Outputs     : new file descriptor or -1 if error

***********************************************************************/

extern int fileOpen( char *name, unsigned int flags );

/**********************************************************************

    Function    : fileWrite
    Description : Write specified number of bytes starting at the current file index
    Inputs      : fd - file descriptor
                  buf - buffer to write
                  bytes - number of bytes to write
    Outputs     : number of bytes written

***********************************************************************/

extern int fileWrite( unsigned int fd, char *buf, unsigned int bytes );

/**********************************************************************

    Function    : fileRead
    Description : Read specified number of bytes from the current file index
    Inputs      : fd - file descriptor
                  buf - buffer for placing data
                  bytes - number of bytes to read
    Outputs     : number of bytes read

***********************************************************************/

extern int fileRead( unsigned int fd, char *buf, unsigned int bytes );

/**********************************************************************

    Function    : fsFindDentry
    Description : Retrieve the dentry for this file name
    Inputs      : dir - directory in which to look for name 
                  name - name of the file 
    Outputs     : new dentry or NULL if error

***********************************************************************/

extern dentry_t *fsFindDentry( dir_t *dir, char *name );

/**********************************************************************

    Function    : fsFindFile
    Description : Retrieve the in-memory file for this file name
    Inputs      : dentry - dentry in which to look for name 
                  name - name of the file 
                  flags - flags requested for this file
    Outputs     : new file or NULL if error

***********************************************************************/

extern file_t *fsFindFile( dentry_t *dentry, char *name, unsigned int flags );

/**********************************************************************

    Function    : listDirectory
    Description : print the files in the root directory currently
    Inputs      : none
    Outputs     : number of bytes read

***********************************************************************/

extern void listDirectory( void );



/**********************************************************************

    Function    : fileClose
    Description : close the file associated with the file descriptor
    Inputs      : fd - file descriptor
    Outputs     : none

***********************************************************************/

extern void fileClose( unsigned int fd );

/**********************************************************************

    Function    : fileSeek
    Description : Adjust offset in per-process file entry
    Inputs      : fd - file descriptor
                  index - new offset 
    Outputs     : 0 on success, -1 on failure

***********************************************************************/

extern int fileSeek( unsigned int fd, unsigned int index );

/**********************************************************************

    Function    : fileWrite
    Description : Write specified number of bytes starting at the current file index
    Inputs      : fd - file descriptor
                  buf - buffer to write
                  bytes - number of bytes to write
    Outputs     : number of bytes written

***********************************************************************/

extern int fileWrite( unsigned int fd, char *buf, unsigned int bytes );

