//#include "type.h"

#define MAXTOKENSIZE 64

/************************ globals *****************************/
MINODE *root; 
char pathname[128], parameter[128], *name[128], cwdname[128];
//char names[128][256];

extern int  nnames;
extern char *rootdev, *slash, *dot;
extern int iblock;

extern MINODE minode[NMINODES];
extern MOUNT  mounttab[NMOUNT];
extern PROC   proc[NPROC], *running;
extern OFT    oft[NOFT];

extern char names[64][64];
extern int fd, imap, bmap;
extern int ninodes, nblocks, nfreeInodes, nfreeBlocks, numTokens;

//===================== Functions YOU already have ==========================

int get_block(int fd, int blk, char buf[])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  read(fd, buf, BLOCK_SIZE);
}

int put_block(int fd, int blk, char buf[])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  write(fd, buf, BLOCK_SIZE);
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

  buf[i] &= ~(1 << j);
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

  // read block_bitmap block
  get_block(dev, bmap, buf);

  for (i = 0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 0)
    {
      set_bit(buf, i);
      decFreeInodes(dev);

      put_block(dev, bmap, buf);

      return i+1;
    }
  }
  printf("balloc(): no more free inodes\n");
  return 0;
}

int bdealloc(int dev, int ino)
{
  int i;
  char buf[BLOCK_SIZE];

  // Read in the bmap block to buf
  get_block(dev, bmap, buf);

  for (i = 0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 1)
    {
      clr_bit(buf, i);
      incFreeInodes(dev);

      put_block(dev, bmap, buf);

      return i+1;
    }
  }
  printf("bdealloc(): no inodes to dealloc.\n");

}

int ialloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, imap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}

int idealloc(int dev)
{
  int i;
  char buf[BLOCK_SIZE];

  // Read in the imap block to buf
  get_block(dev, imap, buf);

  for (i = 0; i < ninodes; i++)
  {
    if (tst_bit(buf, i) == 1)
    {
      clr_bit (buf, i);
      incFreeInodes(dev);

      put_block(dev, imap, buf);

      return i+1;
    }
  }
  printf("idealloc(): no inodes to dealloc.\n");
}

int tokenize(char *pathname)
{
  char *token;
  int index = 1;
  
  numTokens = 0;

  if (pathname[0] == '/')       // If path starts with a '/', remove it to simplify tokens
    token++;

  token = strtok(pathname, "/");
  if (strlen(token) > MAXTOKENSIZE)
  {
    printf("Token size too large. Maximum characters allowed is 64.\n");
    return 0;
  }
  strcpy(names[0], token);

  numTokens++;

  while (token = strtok(NULL, "/"))   // Allocate and copy the rest of the tokens
  {
    if (strlen(token) > MAXTOKENSIZE)
    {
      printf("Token size too large. Maximum characters allowed is 64.\n");
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

char *mydirname(char *path)                 // Created a new dirname and basename that will preserve the given path
{
  char *tempPath;                           // Create copy of path
  strcpy(tempPath, path);
  return (dirname(tempPath));               // Call function with copy
}

char *mybasename(char *path)
{
  char *tempPath;
  strcpy(tempPath, path);
  return (basename(tempPath));
}
  
int getino(int *dev, char *pathname)
{
  char buf[BLOCK_SIZE], *cp, temp[256], *token;
  int index = 0, ino, bno;  

  if (strlen(pathname) < 1)
  {
    printf("Please provide a legitimate path.\n");
    return 0;
  }

  // Open the device for READ
  fd = open(dev, O_RDONLY);

  if (fd < 0)
  {
    printf("Failed to open %s.\n", dev);
    return 0;
  }

  // Read in Superblock, verify it's an ext2 fs
  get_block(fd, SUPERBLOCK, buf);
  sp = (SUPER *)buf;

  if (sp->s_magic != SUPER_MAGIC)
  {
    printf("Sorry, %s is not an EXT2 File System.\n", dev);
    return 0;
  }

  // Read in group block, get InodesBeginBlock
  get_block(fd, GDBLOCK, buf);
  gp = (GD *)buf;
  cp = buf;

  iblock = gp->bg_inode_table;

  // Read InodesBeginBlock to get root dir
  
  get_block(fd, iblock, buf);
  ip = (INODE *)buf + 1;    // root dir
  
  // Tokenize pathname
  
  tokenize(pathname); 

  // Search for token, move to dir, 

  get_block(fd, ip->i_block[0], buf);
  cp = (char *)buf;
  dp = (DIR *)buf;

  for (index = 0; index < numTokens; index++)
  {   
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    ino = isearch(ip, temp, index, cp);

    if (!ino) return 0;

    // At this point, a match must have been found for the current token, move to that inode
    
    // Use mailman's algorithm to find inode

    bno = (ino - 1) / 8 + iblock;
    ino = (ino - 1) % 8;

    //get_block(fd, ino, buf);
    get_block(fd, bno, buf);
    ip = (INODE *)buf + ino;    

    get_block(fd, ip->i_block[0], buf);
    cp = (char *)buf;
    dp = (DIR *)cp;

    if (index != numTokens - 1 && !S_ISDIR(ip->i_mode))   // If we are expecting a directory and it isn't one, exit
    {
      printf("%s is not a directory. Aborting.\n", names[index]);
      return 0;
    }   
  }

  // INODE Found! Now just return it's ino

  return ino;
}   

int isearch(INODE *ip, char *name, int index, char *cp)
{
  int i, inode;

  for (i = 0; i < 12; i++)                            // Assumes only 12 direct blocks
  {
    if (strcmp(name, names[index]) == 0) 
    {
      inode = dp->inode;
      printf("Found %s.\n", name);
      return inode;
    }

      cp += dp->rec_len;                              // Otherwise, move on to the next one!
      dp = (DIR *)cp;

      strncpy(name, dp->name, dp->name_len);
      name[dp->name_len] = 0;
    }

  return inode;
}

/*====================== USAGE of getino() ===================================
Given a pathname, if pathname begins with / ==> dev = root->dev;
                  else                          dev = running->cwd->dev;
With mounting (in level-3), dev may change when crossing mounting point(s).
Whenever dev changes, record it in  int *dev  => dev is the FINAL dev reached.
 
int ino = getino(&dev, pathname) essentially returns (dev,ino) of a pathname.
============================================================================*/

int search(MINODE *mip, char *name)
{
  // This function searches the data blocks of a DIR inode (inside an Minode[])
  // for name. You may assume a DIR has only 12 DIRECT data blocks.

  int i, inode;
  char *cp, *dirname, buf[BLOCK_SIZE];
  DIR *dp;
  INODE *node = &(mip->INODE);

  get_block(fd, node->i_block[0], buf);
  cp = (char *)buf;
  dp = (DIR *)buf;

  for (i = 0; i < 12; i++)                            // Assumes only 12 direct blocks
  {
    strncpy(dirname, dp->name, dp->name_len);
    dirname[dp->name_len] = 0;

    if (strcmp(name, dirname) == 0) 
    {
      inode = dp->inode;
      printf("Found %s.\n", name);
      return inode;
    }

      cp += dp->rec_len;                              // Otherwise, move on to the next one!
      dp = (DIR *)cp;
    }

  return 0;
}

/*==============================================================================
Here, I show how to print the entries of a DIR INODE (containined in a minode[])
      YOU modify it to search for a name string in a DIR INODE*/

int print_dir_entries(MINODE *mip, char *name)
{
   int i; 
   char *cp, sbuf[BLOCK_SIZE], dirname[256];
   DIR *dp;
   INODE *ip;

   ip = &(mip->INODE);
   for (i=0; i<12; i++){  // ASSUME DIRs only has 12 direct blocks
       if (ip->i_block[i] == 0)
          return 0;

       //get ip->i_block[i] into sbuf[ ];
       get_block(fd, ip->i_block[i], sbuf);
       dp = (DIR *)sbuf;
       cp = sbuf;
       while (cp < sbuf + BLOCK_SIZE){
          // print dp->inode, dp->rec_len, dp->name_len, dp->name);

          // WRITE YOUR CODE TO search for name: return its ino if found

          strncpy(dirname, dp->name, dp->name_len);           // Copy directory's name to dirname array
          dirname[dp->name_len] = 0;                          // Put null char at the end so it can be used with string.h functions

          if (strcmp(dirname, name) == 0) return dp->inode;   // If it matches the name, return it's inode #

          cp += dp->rec_len;
          dp = (DIR *)cp;
       }
   }
   return 0;
}

//================ Write C code for these NEW functions ================

MINODE *iget(int dev, int ino)
{
  int i, block, offset;
  char buf[BLOCK_SIZE];
  MINODE *mip;
  INODE *ip;
printf("iget1");
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
printf("iget2");

  // If we didn't find any matches, we have to do more work
  for(i = 0; i < NMINODES; i++)
  {
    MINODE *node = &minode[i];

    if (node->refCount == 0)
    {
      mip = node;
      break;
    }
  }
printf("iget3");

  // If the above loop didn't point mip at anything, there must be no vacancy in minode[], print error and return null
  if (!mip)
  {
    printf("No room in minode[].\n");
    return 0;
  }
printf("iget4");

  // If mip is pointing at a vacant minode, start mailman alg.
  block = (ino - 1) / 8 + IBLOCK;
  offset = (ino - 1) % 8;
printf("iget5");

  get_block(dev, block, buf);         // Get block and inode within block based off mailman alg.
  ip = (INODE *) (buf + offset);
printf("iget6");

  mip->INODE = *ip;                   // Copy ip into mip's INODE
printf("iget7");

  mip->dev = dev;                     // Fill in the rest of mip
  mip->ino = ino;
  mip->refCount = 0;                  // These 0s should already be 0s, but do them just in case
  mip->dirty = 0;
  mip->mounted = 0;
  mip->mountptr = 0;
printf("iget8\n");

  return mip;
}

int iput(MINODE *mip) //This function releases a Minode[] pointed by mip.
{ 
  int block, offset, fd;
  char buf[BLOCK_SIZE];
  INODE *ip;

  // First, decremement mip's refCount;
  mip->refCount -= 1;

  // If mip's refCount is still above 0, leave it and return
  if (mip->refCount > 0)  return;

  // If mip isn't dirty, there's nothing else to do, return
  if (mip->dirty == 0)  return;

  // Otherwise, we need to write the INODE back to disk
  block = (mip->ino - 1) / 8 + IBLOCK;
  offset = (mip->ino - 1) % 8;

  get_block(mip->dev, block, buf);              // Get the INODES' block
  ip = (INODE *) (buf + offset);                // Get the inode within the block

  memcpy(buf + offset, &mip->INODE, 128);       // Copy the new inode data into block from mip

  fd = mip->dev;
  lseek(fd, (long)block * BLOCK_SIZE, 0);
  write(fd, buf, BLOCK_SIZE);
} 

int findmyname(MINODE *parent, int myino, char *myname) 
{
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;
  int i;

  get_block(parent->dev, parent->INODE.i_block[0], buf);
  cp = (char *)buf;
  dp = (DIR *)cp;

  for (i = 0; i < 12; i++)                      // Assumes only 12 direct blocks
  {
    if (dp->inode == myino)                     // See if dir's inode matches given ino 
    {
      strncpy(myname, dp->name, dp->name_len);  // If so, copy the name and return 
      return 1;
    }

      cp += dp->rec_len;                        // Otherwise, move on to the next one!
      dp = (DIR *)cp;
  }

  return 0;                                     // Return 0 if no match was found
}

int findino(MINODE *mip, int *myino, int *parentino)
{
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
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

  mp = iget(dev, ino);

  if (S_ISDIR(mp->INODE.i_mode)) return 1;

  return 0;
}


