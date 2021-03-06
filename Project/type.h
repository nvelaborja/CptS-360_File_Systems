#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // NOTE: Ubuntu users MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLOCK_SIZE     1024
#define INO_PER_BLOCK     8

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define ROOT_INODE        2
#define BMAP              8
#define IMAP              9
#define IBLOCK           10

// Default dir and regulsr file modes
#define DIR_MODE    0040777
#define FILE_MODE   0100644
#define SYM_MODE    0120777
#define SUPER_MAGIC  0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2

// Table sizes
#define NMINODES        100
#define NMOUNT           10
#define NPROC            10
#define NFD              10
#define NOFT            100

// Mount status
#define BUSY              1

// Define colors for output
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

// Open File Table
typedef struct oft{
  int   mode;
  int   refCount;
  struct minode *inodeptr;
  int   offset;
}OFT;

// PROC structure
typedef struct proc{
  int   uid;
  int   pid, ppid, gid;
  int   status;
  struct minode *cwd;
  OFT   *fd[NFD];
  struct proc *next;
  struct proc *child;
  struct proc *parent;
  struct proc *sibling;
}PROC;
      
// In-memory inodes structure
typedef struct minode{    
  INODE INODE;               // disk inode
  int   dev, ino;
  int   refCount;
  int   dirty;
  int   mounted;
  struct mount *mountptr;
}MINODE;

// Mount Table structure
typedef struct mount{
  int    dev;
  int    nblocks,ninodes;
  int    bmap, imap, iblock;
  MINODE *mounted_inode;
  char   name[64]; 
  char   mount_name[64];
  int    busy;
}MOUNT;

