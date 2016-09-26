#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // NOTE: Ubuntu users MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

#define MAXTOKENSIZE 64

/*********************** protoypes ****************************/

int get_block(int fd, int blk, char buffer[]);
int put_block(int fd, int blk, char buf[]);
int tst_bit(char *buf, int bit);
int set_bit (char *buf, int bit);
int clr_bit (char *buf, int bit);
int decFreeInodes (int dev);
int incFreeInodes (int dev);
int balloc(int dev);
int bdealloc(int dev, int ino);
int ialloc(int dev);
int idealloc(int dev, int ino);
int tokenize(char *pathname);
char *mydirname(char *path);
char *mybasename(char *path);
int getino(int dev, char *pathname);
int search(INODE *ip, char *name);
int print_dir_entries(MINODE *mip);
MINODE *iget(int dev, int ino);
int iput(MINODE *mip);
int findmyname(MINODE *parent, int myino, char *myname);
int findino(MINODE *mip, int *myino, int *parentino);
int isDir(int ino);
int displayMINODES();
int displayBlock(char *block);

/************************ globals *****************************/
extern MINODE *root; 
extern char pathname[128], parameter[128], *name[128], cwdname[128], parentPth[128];
extern int  nnames;
extern char *rootdev, *slash, *dot;
extern int iblock;
extern MINODE minode[NMINODES];
extern MOUNT  mounttab[NMOUNT];
extern PROC   proc[NPROC], *running;
extern OFT    oft[NOFT];
extern char names[64][64];
extern int fd, imap, bmap, numTokens;
extern int ninodes, nblocks, nfreeInodes, nfreeBlocks, numTokens, DEBUG;

int get_block(int fd, int blk, char buffer[])
{
  if (DEBUG) printf(YELLOW "Enter get_block().\n\tGiven fd: %d\n\tGiven blk: %d\n\tGiven buf: %x\n" RESET, fd, blk, buffer);

  lseek(fd, (long)blk*BLOCK_SIZE, SEEK_SET);
  read(fd, buffer, BLOCK_SIZE);

  return 1;
}

int put_block(int fd, int blk, char buf[])
{
  lseek(fd, (long)blk*BLOCK_SIZE, SEEK_SET);
  write(fd, buf, BLOCK_SIZE);

  return 1;
}

int tst_bit(char *buf, int bit)
{
  int i, j;

  i = bit / 8;  j = bit % 8;
  
  if (buf[i] & (1 << j))
    return 1;
  return 0;
}

int set_bit (char *buf, int bit)
{
  int i, j;
  
  i = bit / 8;  j = bit % 8;
  
  buf[i] |= (1 << j);
}

int clr_bit (char *buf, int bit)
{
  int i, j;

  i = bit / 8;  j = bit % 8;

  buf[i] &= (0 << j);
}

int decFreeInodes (int dev)
{
  char buf[BLOCK_SIZE];

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int incFreeInodes (int dev)
{
  char buf[BLOCK_SIZE];

  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, SUPERBLOCK, buf);

  get_block(dev, GDBLOCK, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, GDBLOCK, buf);
}

int balloc(int dev)
{
  int i;
  char buf[BLOCK_SIZE];

  if (DEBUG) printf(YELLOW"Enter balloc(). Given dev: %d"RESET, dev);

  // read block_bitmap block

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  bmap = gp->bg_block_bitmap;

  get_block(dev, bmap, buf);

  for (i = 0; i < nblocks; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      decFreeInodes(dev);

      put_block(dev, bmap, buf);

      return i+1;
    }
  }
  printf(RED"balloc(): no more free inodes\n" RESET);
  return 0;
}

int bdealloc(int dev, int ino)
{
  int i;
  char buf[BLOCK_SIZE], copy[BLOCK_SIZE];

  if (DEBUG) printf(YELLOW"Enter bdealloc(). Given dev/ino/bmap: %d/%d/%d"RESET, dev, ino, bmap);

  get_block(dev, bmap, buf);
  memcpy(copy, buf, BLOCK_SIZE);

  clr_bit(buf, ino);
  incFreeInodes(dev);
  put_block(dev, bmap, buf);

  // Check to see if anything was changed
  get_block(dev, bmap, buf);
  if (memcmp(buf, copy, BLOCK_SIZE) == 0)
  {
    printf(RED"NOTHING CHANGED IN BDEALLOC"RESET);
  }

  return 1;
}

int ialloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];

  if (DEBUG) printf(YELLOW"Enter ialloc(). Given dev: %d"RESET, dev);

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  ninodes = sp->s_inodes_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  imap = gp->bg_inode_bitmap;

  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf(RED"ialloc(): no more free inodes\n"RESET);
  return 0;
}

int idealloc(int dev, int ino)
{
  int i;
  char buf[BLOCK_SIZE], copy[BLOCK_SIZE];

  if (DEBUG) printf(YELLOW"Enter idealloc(). Given dev/ino/imap: %d/%d/%d"RESET, dev, ino, imap);

  get_block(dev, imap, buf);
  memcpy(copy, buf, BLOCK_SIZE);

  clr_bit(buf, ino);
  incFreeInodes(dev);
  put_block(dev, imap, buf);

  // Check to see if anything was changed
  get_block(dev, bmap, buf);
  if (memcmp(buf, copy, BLOCK_SIZE) == 0)
  {
    printf(RED"NOTHING CHANGED IN BDEALLOC"RESET);
  }

  return 1;
}

int tokenize(char *pathname)
{
  char *token, path[256];
  int index = 1;
  
  numTokens = 0;

  strcpy(path, pathname);                                   // Make copy of pathname to preserve it

  if (DEBUG) printf(YELLOW"Enter tokenizePath().\n\tGiven Path: %s\n"RESET, path);

  if (strlen(pathname) == 0)
  {
    printf(RED"Can't tokenize empty string.\n"RESET);
    return 0;
  }

  if (path[0] == '/')                                       // If path starts with a '/', remove it to simplify tokens
    token++;

  token = strtok(path, "/");
  if (strlen(token) > MAXTOKENSIZE)
  {
    printf(RED"Token size too large. Maximum characters allowed is 64.\n"RESET);
    return 0;
  }
  strcpy(names[0], token);

  if (DEBUG) printf(YELLOW"\tToken 1: %s\n"RESET, token);

  numTokens++;

  while (token = strtok(NULL, "/"))                         // Allocate and copy the rest of the tokens
  {
    if (DEBUG) printf(YELLOW"\tToken %d: %s\n"RESET, index + 1, token);

    if (strlen(token) > MAXTOKENSIZE)
    {
      printf(RED"Token size too large. Maximum characters allowed is 64.\n"RESET);
      return 0;
    }

    strcpy(names[index], token);
    numTokens++;
    index++;
  }

  // Blank out the rest of the names so old tokenizations won't interfere with new tasks
  while (index < MAXTOKENSIZE)
  {
    memset(names[index], 0, MAXTOKENSIZE);
    index++;
  }

  return 1;
}

char *mydirname(char *path)                               // Created a new dirname and basename that will preserve the given path
{
  char tempPath[256];                                     // Create copy of path
  strcpy(tempPath, path);
  return (dirname(tempPath));                             // Call function with copy
}

char *mybasename(char *path)
{
  char tempPath[256];
  strcpy(tempPath, path);
  return (basename(tempPath));
}
  
int getino(int dev, char *pathname)
{
  char buf[BLOCK_SIZE], buf2[BLOCK_SIZE], *cp, temp[256], *token;
  int index = 0, ino, bno, offset;  
  GD  *gp;
  SUPER  *sp;
  INODE *ip;
  DIR *dp;

  if (DEBUG) printf(YELLOW"\nEnter getino()\n"RESET);

  if (dev < 0)
  {
    printf(RED"Device Error.\n"RESET);
    return 0;
  }

  if (strlen(pathname) == 0)
  {
    printf(RED"Empty path.\n");
    return 0;
  }

  if (strcmp(pathname, "/") == 0)
  {
    return root->ino;
  }
  
  if (DEBUG) printf(YELLOW"Reading Super Block.\n"RESET);               // Read in Superblock, verify it's an ext2 fs

  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;

  if (sp->s_magic != 0xEF53)
  {
    printf(RED"Sorry, %d is not an EXT2 File System.\n"RESET, dev);
    exit(1);
  }

  if (DEBUG) printf(YELLOW"Reading Group Descriptor Block.\n"RESET);    // Read in group block, get InodesBeginBlock

  get_block(dev, GDBLOCK, buf);
  gp = (GD *)buf;
  cp = buf;

  if (DEBUG) printf(YELLOW"Reading INodeBeginBlock.\n"RESET);           // Read InodesBeginBlock to get root dir

  if (pathname[0] == '/')
    ip = &root->INODE;
  else
    ip = &running->cwd->INODE;

  if (DEBUG) printf(YELLOW"\t&ip of INodeBeginBlock: %d\n\n"RESET, &ip);
  
  tokenize(pathname);                                                   // Tokenize path

  if (DEBUG) 
  {
    printf(YELLOW"\tTokens:\n"RESET);
    for (index = 0; index < numTokens; index++)
      printf(YELLOW"\t%d: %s\n"RESET, index + 1, names[index]);

    printf(YELLOW"\t&ip of INodeBeginBlock: %d\n\n"RESET, &ip);
  }

  if (DEBUG) printf(YELLOW"Begin token searching.\n"RESET);
  if (DEBUG) printf(YELLOW"\tbuf: \n"RESET);

  for (index = 0; index < numTokens; index++)                           // Search for token
  {
    if (DEBUG) printf(YELLOW"Loop iteration %d:\n"RESET, index + 1);

    ino = 0;
    bno = 0;

    ino = search(ip, names[index]);

    if (DEBUG) printf(YELLOW"\tino from search: %d\n"RESET, ino);

    if (!ino)
    {
      if (DEBUG) printf(RED"Search failure.\n"RESET);
      return 0;
    }

    // At this point, a match must have been found for the current token, move to that inode

    bno = (ino - 1) / 8 + iblock;
    offset = (ino - 1) % 8;

    if (DEBUG) printf(YELLOW"\toffset after mma: %d\n"RESET, offset);
    if (DEBUG) printf(YELLOW"\tbno after mma: %d\n"RESET, bno);

    get_block(dev, bno, buf2);
    ip = (INODE *)buf2 + offset; 

    if (DEBUG) printf(YELLOW"\t&ip after mma: %d\n"RESET, ip);
    if (DEBUG) printf(YELLOW"\tInode mode: %d\n\n"RESET, ip->i_mode);

    if (DEBUG) printf(YELLOW"Checking to see if inode is a directory.\n"RESET);

    if (index != numTokens - 1 && !isDir(ino))                          // If we are expecting a directory and it isn't one, exit
    {
      if (DEBUG) printf(RED"\t%s is not a directory. Aborting.\n"RESET, names[index]);
      return 0;
    }   
  }

  // INODE Found! Now just return its ino
  if (DEBUG) printf(YELLOW"\nNode Found!\n");
  if (DEBUG) printf("\ti_mode : \t%d\n", ip->i_mode);
  if (DEBUG) printf("\ti_uid : \t%d\n", ip->i_uid);
  if (DEBUG) printf("\ti_size : \t%d\n", ip->i_size);
  if (DEBUG) printf("\ti_gid : \t%d\n", ip->i_gid);
  if (DEBUG) printf("\ti_links_count : %d\n", ip->i_links_count);
  if (DEBUG) printf("\tReturning ino (%d)\n\n"RESET, ino);

  return ino;
}   

int search(INODE *ip, char *name)
{
  int i, inode;
  char buff[BLOCK_SIZE];
  INODE *pip = ip;

  if (DEBUG) printf(YELLOW"Enter search(). Looking for: %s\n"RESET, name);
  if (DEBUG) printf(YELLOW"\tGiven &ip: %d\n\tGiven name: %s\n"RESET, &ip, name);
  if (DEBUG) printf(YELLOW"\tInitial buf: %x\n"RESET, buff);

    for (i = 0; i < 12; i++)                                            // Assume only 12 direct blocks
    {
      char *cp, dirname[256];
      DIR *dp;

      get_block(running->cwd->dev, (int)pip->i_block[i], buff);

      if (DEBUG) printf(YELLOW"\tBuf from ip->i_block[%d]: %x\n"RESET, i, buff);

      cp = buff;
      dp = (DIR *)cp;

      if (DEBUG) printf(YELLOW"\tdp->inode: %d\n"RESET, dp->inode);

      if (DEBUG) printf(YELLOW"\tBegining Scan...\n"RESET);

      while (cp < buff + BLOCK_SIZE)
      {
        strncpy(dirname, dp->name, dp->name_len);
        dirname[dp->name_len] = 0;

        if (!dp->inode) return 0;

        if (DEBUG) printf(YELLOW"\t\tCurrent dir: %s (%d)\n"RESET, dp->name, dp->inode);

        if (strcmp(dirname, name) == 0)                                 // Match found!
        {
         if (DEBUG) printf(YELLOW"\t\tFound %s! It's ino is %d.\n\n"RESET, name, dp->inode);
         return dp->inode;
        }

        cp += dp->rec_len;                                              // If we didn't find a match, move on to the next one
        dp = (DIR *)cp;
      }

      if (DEBUG) printf(RED"\tCouldn't find %s. Are you sure it exists?\n\n"RESET, name);

      return 0;
    }

  return inode;
}

int print_dir_entries(MINODE *mip)
{
  int inode, i, j;
  char name[MAXTOKENSIZE * 2], cwd[MAXTOKENSIZE * 2], buf[BLOCK_SIZE], *cp, ftime[64];
  DIR *dp;
  INODE *ip;
  
  if (DEBUG) printf(YELLOW "Enter print_dir_entries(). dev/ino: %d/%d\n" RESET, mip->dev, mip->ino);

  // Directory
  ip = &(mip->INODE);
  for (i = 0; i < 12; i++)                                            // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] == 0) break;                                   // Break loop if we hit the end of entries

    get_block(mip->dev, (int)ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLOCK_SIZE)
    {
      if (dp->rec_len == 0)                                           // If current node has no parent, we must have reached root
      {
        break;
      }

      // Print entry name
      strncpy(name, dp->name, dp->rec_len);

      if (dp->file_type == EXT2_FT_DIR) printf(GREEN"%s\t"RESET, name);
      else if (dp->file_type != EXT2_FT_REG_FILE) printf(CYAN"%s\t"RESET, name);
      else printf("%s\t", name);

      // Move cp/dp to the next entry
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }

  if (mip) iput(mip);

  printf("\n");
}

MINODE *iget(int dev, int ino)
{
  int i, block, offset;
  char buf[BLOCK_SIZE];
  MINODE *mip;
  INODE *ip;

  if (DEBUG) printf(YELLOW "Enter iget(). dev/ino: %d/%d\n" RESET, dev, ino);

  // First search through inodes to see if the minode already exists
  for (i = 0; i < NMINODES; i++)
  {
    MINODE *node = &minode[i];    // grab the indexed MINODE to shorten syntax in this loop

    if (node->refCount > 0 && node->dev == dev && node->ino == ino)
    {
      // Match found! Just increment it's refCount, return a pointer to it
      node->refCount += 1;
      return node;
    }
  }

  // If we didn't find any matches, find the first minode with refCount == 0
  for(i = 0; i < NMINODES; i++)
  {
    MINODE *node = &minode[i];

    if (node->refCount == 0)
    {
      mip = node;
      break;
    }
  }

  // If the above loop didn't point mip at anything, there must be no vacancy in minode[], print error and return null
  if (!mip)
  {
    printf(RED"No room in minode[].\n"RESET);
    return 0;
  }

  // If mip is pointing at a vacant minode, start mailman alg.
  block = (ino - 1) / 8 + IBLOCK;
  offset = (ino - 1) % 8;

  get_block(dev, block, buf);         // Get block and inode within block based off mailman alg.
  ip = (INODE *) buf + offset;

  //mip->INODE = *ip;                 // Copy ip into mip's INODE
  //copyINODE(&(mip->INODE), ip);
  //memcpy(&mip->INODE, ip, sizeof(INODE));
  memcpy(&mip->INODE, (((INODE *)buf) + offset), sizeof(INODE));

  mip->dev = dev;                     // Fill in the rest of mip
  mip->ino = ino;
  mip->refCount = 1;                  
  mip->dirty = 0;                     // These 0s should already be 0s, but do them just in case
  mip->mounted = 0;
  mip->mountptr = 0;

  return mip;
}

int iput(MINODE *mip) //This function releases a Minode[] pointed by mip.
{ 
  int block, offset, i;
  char buf[BLOCK_SIZE];
  INODE *ip;

  if (DEBUG) printf(YELLOW "Enter iget(). dev/ino: %d/%d\n" RESET, mip->dev, mip->ino);

  // First, decremement mip's refCount;
  mip->refCount -= 1;

  // This stuff not working correctly, I think mip's refCount is inaccurate.
  /*// If mip's refCount is still above 0, leave it and return
  if (mip->refCount > 0)  
  {
    if (DEBUG) printf(YELLOW "mip ref count > 0.\n" RESET);
    return 0;
  }

  // If mip isn't dirty, there's nothing else to do, return
  if (mip->dirty == 0) 
  {
    if (DEBUG) printf(YELLOW "mip not dirty.\n" RESET);
    return 0;
  }*/

  // Otherwise, we need to write the INODE back to disk
  block = (mip->ino - 1) / 8 + IBLOCK;
  offset = (mip->ino - 1) % 8;

  get_block(mip->dev, block, buf);              // Get the INODES' block
  ip = (INODE *) buf + offset;                  // Get the inode within the block

  //memcpy(ip, &mip->INODE, 128);               // Copy the new inode data into block from mip
  ip->i_mode = mip->INODE.i_mode;               // I can't tell if the memcpy is working so I'm doing the copy manually
  ip->i_uid = mip->INODE.i_uid;
  ip->i_size = mip->INODE.i_size;
  ip->i_atime = mip->INODE.i_atime;
  ip->i_ctime = mip->INODE.i_ctime;
  ip->i_mtime = mip->INODE.i_mtime;
  ip->i_dtime = mip->INODE.i_dtime;
  ip->i_gid = mip->INODE.i_gid;
  ip->i_links_count = mip->INODE.i_links_count;

  for(i = 0; i < 12; i++)
  {
    ip->i_block[i] = mip->INODE.i_block[i];
  }

  put_block(mip->dev, block, buf);

  if (DEBUG) printf(YELLOW "iput successful.\n" RESET);

  return 1;
} 

int findmyname(MINODE *parent, int myino, char *myname) 
{
  char buf[BLOCK_SIZE], *cp, temp[256];
  DIR *dp;
  int i;

  for (i = 0; i < 12; i++)                      // Assumes only 12 direct blocks
  {
    if (parent->INODE.i_block[i] == 0) continue;

    get_block(parent->dev, (int)parent->INODE.i_block[i], buf);
    cp = (char *)buf;
    dp = (DIR *)cp;

    strncpy(temp, dp->name, dp->name_len);      // If so, copy the name and return 
    temp[dp->name_len] = 0;

    while (cp < (buf + BLOCK_SIZE))
    {
      if (dp->inode == myino)                   // See if dir's inode matches given ino 
      {
        strcpy(myname, temp);
        
        return 1;
      }
      
      cp += dp->rec_len;                        // Otherwise, move on to the next one!
      dp = (DIR *)cp;
      strncpy(temp, dp->name, dp->name_len);    // If so, copy the name and return 
      temp[dp->name_len] = 0;
    }
  }

  return 0;                                     // Return 0 if no match was found
}

int findino(MINODE *mip, int *myino, int *parentino)
{
  char buf[BLOCK_SIZE], *cp;
  INODE *ip = &mip->INODE;
  DIR *dp;

  get_block(mip->dev, (int)mip->INODE.i_block[0], buf);
  cp = (char *)buf;
  dp = (DIR *)cp;

  *myino = dp->inode;

  cp+= dp->rec_len;
  dp = (DIR *)cp;

  *parentino = dp->inode;
  
  return 1;
}

int isDir(int ino)
{
  INODE *ip;
  MINODE *mp;

  mp = iget(running->cwd->dev, ino);
  if (S_ISDIR(mp->INODE.i_mode)) return 1;
  iput(mp);

  return 0;
}

int isReg(int ino)
{
  INODE *ip;
  MINODE *mp;

  mp = iget(running->cwd->dev, ino);
  if (S_ISREG(mp->INODE.i_mode)) return 1;
  iput(mp);

  return 0;
}

int getParentPath(char *pathname)
{
  char pathCopy[256];
  int i = strlen(pathname) - 1;

  if (DEBUG) printf(YELLOW "Enter getParentPath\n" RESET);

  strcpy(pathCopy, pathname);

  if (DEBUG) printf(YELLOW "Given Path: %s\n" RESET, pathname);

  if (pathCopy[strlen(pathCopy) - 1] == '/')                    // If the given path has a '/' at the end of it, remove it
  {
    pathCopy[strlen(pathCopy)] = 0;                             // Cut off last character
    i--;                                                        // Decrement last_char index
  }

  while (i > 0)                                                 // Keep cutting off characters until we reach the next '/' 
  {
    if (pathCopy[i] == '/')
    {
      pathCopy[i] = 0;
      strcpy(parentPth, pathCopy);
      return 1;
    }

    pathCopy[i] = 0;
    i--;
  }

  return 0;
}

int emptyDir(INODE *ip, int dev)
{
  char *cp, buff[BLOCK_SIZE];
  DIR *dp;

  if (ip->i_links_count > 2)
  {
    return 0;
  }

  get_block(dev, (int)ip->i_block[0], buff);                    // Get buf to the beginning of the dir entries
  cp = buff;
  dp = (DIR *)cp;

  cp += dp->rec_len;                                            // Step to the .. entry
  dp = (DIR *)cp;

  // If the .. entry is bigger than its ideal length, it must be the last entry 
  if (dp->rec_len == (4 * ((8 + dp->name_len + 3) / 4))) 
  {
    return 0;
  }

  return 1;
}

int copyINODE(INODE *dest, INODE *source)
{
  int i;

  if (DEBUG) printf(YELLOW "Enter copyINODE.\n" RESET);

  if (dest == 0 || source == 0)
  {
    if (DEBUG) printf(YELLOW "INODE copy failure.\n" RESET);
    return 0;
  }

  dest->i_mode = source->i_mode;
  dest->i_uid = source->i_uid;
  dest->i_size = source->i_size;
  dest->i_atime = source->i_atime;
  dest->i_ctime = source->i_ctime;
  dest->i_mtime = source->i_mtime;
  dest->i_dtime = source->i_dtime;
  dest->i_gid = source->i_gid;
  dest->i_links_count = source->i_links_count;
  dest->i_blocks = source->i_blocks;
  dest->i_flags = source->i_flags;
  for (i = 0; i < 15; i++)
    dest->i_block[i] = source->i_block[i];
  dest->i_file_acl = source->i_file_acl;
  dest->i_dir_acl = source->i_dir_acl;
  dest->i_faddr = source->i_faddr;

  return 1;
}

int displayMINODES()
{
  int i;
  MINODE *mp;

  if (!DEBUG) return 0; 

  printf(CYAN"**************************************************\n"RESET);
  printf(RESET"**************************************************\n"RESET);

  for (i = 0; i < NMINODES; i++)
  {
    mp = &minode[i];

    if (!mp) continue;
    if (mp->dev == 0 && mp->ino == 0 && mp->refCount == 0) continue;

    printf(CYAN"~ MINODE %d\n"RESET, i);
    printf(CYAN"\tdev: %d   ino: %d\n"RESET, mp->dev, mp->ino);
    printf(CYAN"\trefCount: %d   dirty: %d   mounted: %d\n"RESET, mp->refCount, mp->dirty, mp->mounted);
    printf(CYAN"\tINODE:\n"RESET);
    printf(CYAN"\t\tmode: %o   uid: %d   size: %d\n"RESET, mp->INODE.i_mode, mp->INODE.i_uid, mp->INODE.i_size);
  }

  printf(RESET"**************************************************\n"RESET);
  printf(CYAN"**************************************************\n\n"RESET);

  return 1;
}

int displayBlock(char *block)
{
  int i, j;

  for (i = 0; i < BLOCK_SIZE; i++)
  {
    for (j = 0; j < INO_PER_BLOCK; i++)
    {
      if (DEBUG) printf(YELLOW "%d" RESET, block[i] & 1 << j);
    }
    printf(" ");
  }
  return 1;
}