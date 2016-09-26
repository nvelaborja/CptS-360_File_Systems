#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>  
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

extern char *t1;
extern char *t2;
extern char parentPth[128];
extern int isPath, isParameter, DEBUG;

/************************************************************\
                          BUGS / TODO
\************************************************************\
  - In rmdir and rm, checking to see if file is busy doesn't
      work quite right.
  - using pwd at the beginning of the command line goofs when
      the pwd function is used

*************************************************************/

/************************************************************\
                         PROTOTYOPES
\************************************************************/

int make_dir();
int mymkdir(MINODE *pip, char *name, int dev);
int change_dir();
int pwd();
int rpwd(MINODE *node, int ino);
int list_dir();
int creat_file();
int rmdir();
int rm_file();
int mystat();
int chmod_file();
int do_touch(char *path);
int open_file();
int close_file();
int read_file();
int write_file();
int cat_file();
int cp_file();
int mv_file();
int lseek_file();
int mount();
int umount();
int enter_name(MINODE *pip, int myino, char *myname);
int menu();
int quit();
int getParentPath(char *pathname);
int pfd();
int rewind_file();
int pm();
int access_file();
int chown_file();
int cs();
int do_fork();
int do_ps();
int do_kill();
int sync();
int symlink();
int link();
int unlink();
int truncate (MINODE *mp);


/************************************************************\
                      LEVEL 1 FUNCTIONS
\************************************************************/

int make_dir()
{
  MINODE *mip, *pip;
  char parent[256], child[256];
  int pino, dev;
  
  if (DEBUG) printf(YELLOW"Enter make_dir.\n"RESET);
  if (DEBUG) printf(YELLOW "path: %s\n" RESET, pathname);

  // We need to create a dir at the location specified
  dev = running->cwd->dev;

  strcpy(parent, mydirname(pathname));                                // Grab parent and child names from path
  strcpy(child, mybasename(pathname));

  if (DEBUG) printf(YELLOW "parent: %s\n" RESET, parent);
  if (DEBUG) printf(YELLOW "child: %s\n" RESET, child);

  if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
  {
    if (pathname[0] == '/') pino = root->ino;
    else pino = running->cwd->ino;
  }
  else  pino = getino(dev, parent);                                   // Grab inode numbers of parent and child

  pip = iget(dev, pino);
  
  if (DEBUG) printf(YELLOW "pino: %d\n" RESET, pino);


  if (!isDir(pino))                                                   // If parent isn't a dir, leave function
  {
    printf(RED"%s not a directory.\n"RESET, parent);
    iput(pip);
    return 0;
  }
  if (search(&pip->INODE, child))                                     // If child already exists in parent, leave function
  {
    printf(RED"%s already exists.\n"RESET, child);
    iput(pip);
    return 0;
  }

  mymkdir(pip, child, dev);                                           // Create the directory in parent's i_blocks

  pip->INODE.i_links_count += 1;                                      // Increment parent's link count

  do_touch(parent);                                                   // Touch parent to update access and modification times

  pip->dirty = 1;                                                     // Set parent to dirty so it will have to be saved

  iput(pip);                                                          // Write MINODE to disk

  return 1;
}

int mymkdir(MINODE *pip, char *name, int dev)
{
  int ino, bno, i;
  MINODE *mip;
  INODE *ip;
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  bno = balloc(dev);
  ino = ialloc(dev);                                                  // Allocate new ino and bno
  mip = iget(dev, ino);                                               // create new MINODE for the new ino 
  ip = &mip->INODE;
  ip->i_mode = DIR_MODE;                                              // Set all of its information
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = BLOCK_SIZE;
  ip->i_links_count = 2;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = bno;
  for (i = 1; i < 15; i++)
    ip->i_block[i] = 0;

  mip->dirty = 1;
  iput(mip);                                                          // Write MINODE to disk

  // Create . and .. dir entries
  get_block(running->cwd->dev, bno, buf);
  cp = buf;
  dp = (DIR *)cp;
  dp->inode = ino;
  dp->name_len = 1;
  dp->file_type = EXT2_FT_DIR;
  strncpy(dp->name, ".", 1);
  dp->rec_len = 4 * ((8 + dp->name_len + 3) / 4);
  cp += dp->rec_len;
  dp = (DIR *)cp;
  dp->inode = pip->ino;
  dp->name_len = 2;
  dp->file_type = EXT2_FT_DIR;
  strncpy(dp->name, "..", 2);
  dp->rec_len = BLOCK_SIZE - (4 * ((8 + 1 + 3) / 4));

  // Write buf to the disk block bno
  put_block(running->cwd->dev, bno, buf);

  // Enter name ENTRY into parent's directory 
  enter_name(pip, ino, name);

  return 1;
}

int change_dir()
{
  MINODE *mp;
  INODE *ip;
  int ino;

  if (DEBUG) printf(YELLOW"Enter change_dir.\n"RESET);

  // We need to change cwd to root if no pathname is provided
  if (!isPath)
  {
    iput(running->cwd);                                               // Record any changes to cwd before we move
    running->cwd = root;
    return 1;
  }

  // Otherwise change cwd to whatever path provided
  ino = getino(running->cwd->dev, pathname);

  if (!ino)
  {
    printf(RED"%s could not be found.\n"RESET, pathname);
    return 0;
  }

  mp = iget(running->cwd->dev, ino);

  if (!isDir(mp->ino))
  {
    printf(RED"%s is not a directory.\n"RESET, pathname);
    return 0;
  }

  iput(running->cwd);                                                 // Record any changes to cwd before we move
  running->cwd = mp;

  return 1;
}

int pwd()
{
  MINODE *mp, *mpp;
  int ino, pino;
  char temp[256], path[256], tempPath[256];

  if (DEBUG) printf(YELLOW"Enter pwd.\n"RESET);

  mp = running->cwd;

  if (mp->ino == root->ino)
  {
    printf("/");
    return 1;
  }

  findino(mp, &ino, &pino);
  if (DEBUG) printf(YELLOW"ino and pino returned from findino: %d %d\n"RESET, ino, pino);
  
  while (ino != pino)                                                 // Root dir will have ino == pino... I think
  {
    mpp = iget(mp->dev, pino);

    findmyname(mpp, ino, temp);

    strcpy(tempPath, path);
    strcpy(path, temp);
    strcat(path, "/");
    strcat(path, tempPath);

    findino(mpp, &ino, &pino);
  }

  strcpy(tempPath, path);
  strcpy(path, "/");
  strcat(path, tempPath);

  printf("%s", path);
}

int rpwd(MINODE *node, int ino)                                       // Recursive pwd printing function, not currently in use
{
  MINODE *mpp;
  DIR *dp;
  char *cp, buf[BLOCK_SIZE], name[256];

  if (DEBUG) printf(YELLOW "Given node: %d  Given ino: %d\n" RESET, node->ino, ino);  

  // Get the dir's parent (..)
  if (node->ino == root->ino)
  {
    printf("/");
  }

  get_block(node->dev, (int)node->INODE.i_block[0], buf);             // Get the first block then move to second entry (..);
  cp = buf;
  dp = (DIR *)cp;
  cp += dp->rec_len;
  dp = (DIR *)cp;

  if (DEBUG) printf(YELLOW"Parent's ino: %s %d\n"RESET, dp->inode);

  if (node->ino != root->ino)                                         // If parent is not root, get parent's MINODE and recursive call
  {
    mpp = iget(node->dev, dp->inode);                                 // Get parent

    rpwd(mpp, node->ino);                                             // Recursive call with parent and current ino
  }

  if (ino)
  {
    while (dp->inode != ino)                                          // Instead, try printing childs name recursively
    {
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;

    printf("%s", name);
    printf("/");
  }

  if (mpp) iput(mpp);

  return 1;
}

int list_dir()
{
  int inode, i, j, dev;
  char name[MAXTOKENSIZE * 2], cwd[MAXTOKENSIZE * 2], buf[BLOCK_SIZE], *cp, ftime[64];
  MINODE *mp, *tmp;
  DIR *dp;
  INODE *ip;
  
  if (DEBUG) printf(YELLOW"Enter list_dir.\n"RESET);

  dev = running->cwd->dev;

  if (strlen(pathname) == 0)                            // No given path, use cwd
  {
    mp = running->cwd;
  }
  else                                                  // Given path, get mp from path
  {
    inode = getino(dev, pathname);                      // Get the inode 
    if (DEBUG) printf(YELLOW"list_dir: inode received from getino: %d\n"RESET, inode);

    mp = iget(dev, inode);                              // Get the MINODE so we can access it's INODE struct
  }

  // File
  if (S_ISREG(mp->INODE.i_mode))                        // If the specified inode is a file, just print the file's info
  {
    mystat();

    return 1;
  }

  // Directory
  ip = &(mp->INODE);
  for (i = 0; i < 12; i++)                              // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] == 0) break;                     // Break loop if we hit the end of entries

    get_block(dev, (int)ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLOCK_SIZE)
    {
      if (dp->rec_len == 0)                             // If current node has no parent, we must have reached root
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

  if (mp) iput(mp);

  printf("\n");
}

int creat_file()
{
  MINODE *mip, *pip;
  char parent[256], child[256];
  int pino, dev;
  
  if (DEBUG) printf(YELLOW"Enter make_dir.\n"RESET);
  if (DEBUG) printf(YELLOW "path: %s\n" RESET, pathname);

  // We need to create a dir at the location specified

  dev = running->cwd->dev;

  strcpy(parent, mydirname(pathname));                                // Grab parent and child names from path
  strcpy(child, mybasename(pathname));

  if (DEBUG) printf(YELLOW "parent: %s\n" RESET, parent);
  if (DEBUG) printf(YELLOW "child: %s\n" RESET, child);

  if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
  {
    if (pathname[0] == '/') pino = running->cwd->ino;
    else pino = root->ino;
  }
  else  pino = getino(dev, parent);                                   // Grab inode numbers of parent and child

  pip = iget(dev, pino);
  
  if (DEBUG) printf(YELLOW "pino: %d\n" RESET, pino);


  if (!isDir(pino))                                                   // If parent isn't a dir, leave function
  {
    printf(RED"%s not a directory.\n"RESET, parent);
    iput(pip);
    return 0;
  }
  if (search(&pip->INODE, child))                                     // If child already exists in parent, leave function
  {
    printf(RED"%s already exists.\n"RESET, child);
    iput(pip);
    return 0;
  }

  mycreat(pip, child, dev);                                           // Create the directory in parent's i_blocks

  do_touch(parent);                                                   // Touch parent to update access and modification times

  pip->dirty = 1;                                                     // Set parent to dirty so it will have to be saved

  iput(pip);                                                          // Write MINODE to disk

  return 1;
}

int mycreat(MINODE *pip, char *name, int dev)
{
  int ino, bno, i;
  MINODE *mip;
  INODE *ip;
  char buf[BLOCK_SIZE], *cp;
  DIR *cur, *prev;

  ino = ialloc(dev);                                                  // Allocate new ino and bno
  
  mip = iget(dev, ino);                                               // create new MINODE for the new ino 
  ip = &mip->INODE;

  ip->i_mode = FILE_MODE;                                             // Set all of its information
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = 0;
  ip->i_links_count = 1;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 0;
  for (i = 0; i < 15; i++)
    ip->i_block[i] = 0;

  mip->dirty = 1;

  iput(mip);                                                          // Write MINODE to disk

  // Enter name ENTRY into parent's directory 
  enter_filename(pip, ino, name);

  return 1;
}

int rmdir()
{
  INODE *ip, *pip;
  MINODE *mp, *pmp;
  char parent[256], child[256];
  int pino, dev, ino, i;

  if (DEBUG) printf(YELLOW"Enter rmdir.\n"RESET);

  dev = running->cwd->dev;

  strcpy(parent, mydirname(pathname));                                // Grab parent and child names from path
  strcpy(child, mybasename(pathname));

  if (DEBUG) printf(YELLOW"parent: %s, child: %s\n"RESET, parent, child);

  // Get inumber from pathname
  ino = getino(dev, pathname);

  if (!ino)
  {
    printf(RED"Couldn't find %s, are you sure it exists?\n"RESET, pathname);
    return 0;
  }

  // Get inode and MINODE
  mp = iget(dev, ino);
  ip = &mp->INODE;

  // Check ownership
  if (running->uid != 0 && running->uid != ip->i_uid)
  {
    printf(RED"Permission denied.\n"RESET);
    return 0;
  }

  // Check base cases: Is a dir, Busy dir, empty
  if (ip->i_mode != DIR_MODE)
  {
    printf(RED"%s not a directory.\n"RESET, pathname);
    return 0;
  }

  if (!emptyDir(ip, dev))
  {
    printf(RED"%s is not empty!\n"RESET, pathname);
    return 0;
  }

  // Deallocate its block and inode
  for (i = 0; i < 12; i++)
  {
    if (ip->i_block[i] == 0) continue;
    bdealloc(mp->dev, mp->ino);
  }

  idealloc(mp->dev, mp->ino);
  iput(mp);

  // Get parent's dir INODE and MINODE
  if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
  {
    if (pathname[0] == '/') pino = running->cwd->ino;
    else pino = root->ino;
  }
  else  pino = getino(dev, parent);                                   // Grab inode numbers of parent and child
  pmp = iget(dev, pino);
  pip = &pmp->INODE;

  if (DEBUG) print_dir_entries(pmp);                                  // Print dir entries before rmchild

  rmchild(pmp, child);                                                // Remove child's entry from parents dir

  pip->i_links_count -= 1;                                            // Decrement parents link count
  getParentPath(pathname);
  do_touch(parentPth);
  pmp->dirty = 1;
  iput(pmp);

  if (DEBUG) print_dir_entries(pmp);     // Print dir entries after to see if it worked

  return 1;
}

int rmchild(MINODE *parent, char *childName)
{
  int i;
  char buff[BLOCK_SIZE], *cp, name[256];
  DIR *dp, *pdp;

  if (DEBUG) printf(YELLOW"Enter rmchild. parent ino: %d, childName: %s\n"RESET, parent->ino, childName);

  // Search through data blocks for the childName entry
  for (i = 0; i < 12; i++)
  {
    if (parent->INODE.i_block[i] == 0)                                      // Assumes no gaps in the data blocks
    {
      if (DEBUG) printf(YELLOW"Empty data blocks.\n"RESET);
      return 0;
    }

    get_block(parent->dev, (int)parent->INODE.i_block[i], buff);            // Get the direct block
    cp = buff;
    dp = (DIR *)cp;
    pdp = dp;

    while (cp < buff + BLOCK_SIZE)                                          // Look through block for childName
    {
      strncpy(name, dp->name, dp->name_len);                                // Copy the dir's name so we can compare it
      name[dp->name_len] = 0;                                               // Terminate string with null char

      if (DEBUG) printf(YELLOW"Current entry: (%s)\n"RESET, name);

      if (strcmp(name, childName) == 0)
      {
        // Found! delete the entry
        if (DEBUG) printf(YELLOW"Match found!\n"RESET);

        int freeSpace = dp->rec_len;

        if (freeSpace > (4 * ((8 + dp->name_len + 3) / 4)))                 // Last entry
        {
          memset(dp, 0, dp->rec_len);
          pdp->rec_len += freeSpace;

          put_block(parent->dev, parent->INODE.i_block[i], buff);
          parent->dirty = 1;
          return 1;
        }

        if (cp == buff && cp + dp->rec_len >= buff + BLOCK_SIZE)
        {
          for (i = i; i < 12; i++)                                          // Shift remaining blocks up one
          {
            parent->INODE.i_block[i] = parent->INODE.i_block[i+1];
          }

          put_block(parent->dev, parent->INODE.i_block[i], buff);
          parent->dirty = 1;
          return 1;
        }
        else if (cp + dp->rec_len < buff + BLOCK_SIZE)
        {
          // Move to last entry
          while(cp < buff + BLOCK_SIZE)
          {
            pdp = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;

            pdp->inode = dp->inode;
            pdp->name_len = dp->name_len;
            pdp->file_type = EXT2_FT_DIR;
            strncpy(pdp->name, dp->name, dp->name_len);

            if (cp + dp->rec_len >= buff + BLOCK_SIZE)
            {
              pdp->rec_len = freeSpace + dp->rec_len;
            }
            else pdp->rec_len = dp->rec_len;

            if (cp + dp->rec_len >= buff + BLOCK_SIZE) break;
          }

          put_block(parent->dev, parent->INODE.i_block[i], buff);
          parent->dirty = 1;
          return 1;
        }

        parent->dirty = 1;
        put_block(parent->dev, parent->INODE.i_block[i], buff);
        return 1;
      }

      // If not found, move dp to next dir entry
      pdp = (DIR *)dp;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }

  return 0;
}

int rm_file()
{
  INODE *ip, *pip;
  MINODE *mp, *pmp;
  char parent[256], child[256];
  int pino, dev, ino, i;

  if (DEBUG) printf(YELLOW"Enter rm_file.\n"RESET);

  dev = running->cwd->dev;

  strcpy(parent, mydirname(pathname));                                // Grab parent and child names from path
  strcpy(child, mybasename(pathname));

  if (DEBUG) printf(YELLOW"Parent: %s, Child: %s\n"RESET, parent, child);

  // Get inumber from pathname
  ino = getino(dev, pathname);

  if (!ino)
  {
    printf(RED"Couldn't find %s, are you sure it exists?\n"RESET, pathname);
    return 0;
  }

  // Get inode and MINODE
  mp = iget(dev, ino);
  ip = &(mp->INODE);

  // Check ownership
  if (running->uid != 0 && running->uid != ip->i_uid)
  {
    printf(RED"Permission denied.\n"RESET);
    return 0;
  }

  // Check base cases: Is a dir, Busy dir, empty
  if (ip->i_mode != FILE_MODE)
  {
    printf(RED"%s not a file.\n"RESET, pathname);
    return 0;
  }

  // Check to see if file is busy ... not sure if this works right
  /*if (mp->refCount > 1)
  {
    printf(RED"%s is busy.\n"RESET, pathname);
    return 0;
  }*/

  // Deallocate its block and inode
  for (i = 0; i < 12; i++)
  {
    if (ip->i_block[i] == 0) continue;
    bdealloc(dev, ip->i_block[i]);
  }

  idealloc(dev, mp->ino);
  iput(mp);

  // Get parent's dir INODE and MINODE
  if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
  {
    if (pathname[0] == '/') pino = running->cwd->ino;
    else pino = root->ino;
  }
  else  pino = getino(dev, parent);                                   // Grab inode numbers of parent and child
  pmp = iget(dev, pino);
  pip = &(pmp->INODE);

  // Remove child's entry from parents dir
  rmchild(pmp, child);

  pip->i_links_count -= 1;
  getParentPath(pathname);
  do_touch(parentPth);
  pmp->dirty = 1;
  iput(pmp);

  return 1;
}

int mystat()
{
  char name[128];
  int ino, dev;
  INODE *ip;
  MINODE *mp;

  if (DEBUG) printf(YELLOW "Enter mystat.\n" RESET);

  dev = running->cwd->dev;

  if (!isPath)
  {
    mp = running->cwd;
    ip = &mp->INODE;
  }
  else
  {
    ino = getino(dev, pathname);

    if (!ino)
    {
      printf(RED "Can't stat %s\n" RESET, pathname);
      return 0;
    }

    mp = iget(dev, ino);
    ip = &mp->INODE;

    strcpy(name, pathname);
    basename(name);
  }

  printf(MAGENTA "File: \'%s\'\n" RESET, name);
  printf(MAGENTA "Size: %d\tMode: %o\n" RESET, ip->i_size, ip->i_mode);
  printf(MAGENTA "Device: %d\tInode: %d\tLinks: \n" RESET, mp->dev, mp->ino, ip->i_links_count);
  printf(MAGENTA "Access Time: %d\n" RESET, ip->i_atime);
  printf(MAGENTA "Modify Time: %d\n" RESET, ip->i_mtime);
  printf(MAGENTA "Change Time: %d\n" RESET, ip->i_ctime);

  if (mp) iput(mp);

  return 1;
}

int chmod_file()
{
  int dev, ino, newMode;
  char path[256];
  INODE *ip;
  MINODE *mp;

  newMode = atoi(pathname);
  strcpy(path, parameter);

  if (DEBUG) printf(YELLOW"Enter chmod_file.\n"RESET);

  dev = running->cwd->dev;

  // Get INODE from pathname
  ino = getino(dev, path);

  if (!ino)
  {
    printf(RED"Couldn't find %s, are you sure it exists?\n"RESET, path);
    return 0;
  }

  mp = iget(dev, ino);
  ip = &(mp->INODE);

  // set i_mode = newMode;
  ip->i_mode = newMode;

  // Write everything to disk.. or whatever -.-
  mp->dirty = 1;
  iput(mp);

  return 1;
}

int chown_file()
{
  int dev, ino, newOwner;
  char path[256];
  INODE *ip;
  MINODE *mp;

  newOwner = atoi(pathname);
  strcpy(path, parameter);

  if (DEBUG) printf(YELLOW"Enter chown_file.\n"RESET);

  dev = running->cwd->dev;

  // Get INODE from pathname
  ino = getino(dev, path);

  if (!ino)
  {
    printf(RED"Couldn't find %s, are you sure it exists?\n"RESET, path);
    return 0;
  }

  mp = iget(dev, ino);
  ip = &(mp->INODE);

  // set i_uid = newOwner;
  ip->i_uid = newOwner;

  // Write everything to disk.. or whatever -.-
  mp->dirty = 1;
  iput(mp);

  return 1;
}

int chgrp_file(int newGroup, char *path)
{
  int dev, ino;
  INODE *ip;
  MINODE *mp;

  if (DEBUG) printf(YELLOW"Enter chgrp_file.\n"RESET);

  dev = running->cwd->dev;

  // Get INODE from pathname
  ino = getino(dev, path);

  if (!ino)
  {
    printf(RED"Couldn't find %s, are you sure it exists?\n"RESET, path);
    return 0;
  }

  mp = iget(dev, ino);
  ip = &(mp->INODE);

  // set i_gid = newGroup;
  ip->i_gid = newGroup;

  // Write everything to disk.. or whatever -.-
  mp->dirty = 1;
  iput(mp);

  return 1;
}

int do_touch(char *path)
{
  INODE *ip;
  MINODE *mp;
  int ino, dev;

  if (DEBUG) printf(YELLOW "Enter do_touch. Given path: %s\n" RESET, path);

  if (strcmp(path, "touch") == 0) strcpy(path, pathname);

  // Get device
  dev = running->cwd->dev;

  if (strlen(path) == 0)                      // Means only a basename was given
  {
    if (pathname[0] == "/") ino = root->ino;
    else ino = running->cwd->ino;
  }
  else ino = getino(dev, path);               // Get ino from pathname
  

  if (DEBUG) printf(YELLOW "Ino returned from getino: %d.\n" RESET, ino);

  // Get INODE from pathname
  mp = iget(dev, ino);

  // Set i_atime and i_mtime to current time
  mp->INODE.i_atime = mp->INODE.i_mtime = time(0L);

  mp->dirty = 1;

  return 1;
}

int link()
{
  // Usage: link oldFileName newFileName

  MINODE *mp, *nmp;
  char oldFile[256], newFile[256], child[256], parent[256], *cpOld, *cpNew, buff[BLOCK_SIZE], oldName[256], newName[256];
  int ino, dev, pino, nino, i, found;
  DIR *dpOld, *dpNew;

  if (DEBUG) printf(YELLOW"Enter link.\n"RESET);

  dev = running->cwd->dev;
 
  if (strlen(pathname) == 0 || strlen(parameter) == 0)
  {
    printf(RED"Please provide source and destination filenames.\n"RESET);
    return 0;
  }

  if (DEBUG) printf(YELLOW"source: %s, dest: %s\n"RESET,pathname, parameter);

  strcpy(oldFile, pathname);
  strcpy(newFile, parameter);

  strcpy(child, mybasename(newFile));
  strcpy(parent, mydirname(newFile));

  if (DEBUG) printf(YELLOW"old file: %s, new file: %s\n"RESET, oldFile, newFile);
  if (DEBUG) printf(YELLOW"child: %s, parent: %s\n"RESET,child, parent);

  ino = getino(dev, oldFile);

  if (isDir(ino))
  {
    printf("%s is a directory. Cannot perform link.\n", oldFile);
    return 0;
  }

  mp = iget(dev, ino);

  // See if parent desitnation exists and destination does not
  if (strcmp(parent, ".") == 0 || strcmp(parent, "/") == 0)
  {
    if (parent[0] == "/") pino = root->ino;
    else pino = running->cwd->ino;
  }
  else pino = getino(dev, newFile);

  if (!pino)
  {
    printf(RED"%s not legitimate path.\n"RESET, newFile);
    return 0;
  }

  if (!isDir(pino))
  {
    printf(RED"%s is not a directory.\n"RESET, parentPth);
    return 0;
  }

  nmp = iget(dev, pino);                    // Get the parent if it passes all tests

  enter_filename(nmp, ino, child);

  mp->INODE.i_links_count += 1;
  nmp->INODE.i_links_count += 1;

  mp->dirty = 1;
  nmp->dirty = 1;

  iput(mp);
  iput(nmp);

  return 1;
}

int unlink()
{
  MINODE *mp, *pmp;
  char file[256], fileCopy[256], *cpOld, *cpNew, buff[BLOCK_SIZE], dirname[256], basename[256];
  int ino, pino, dev;
  DIR *dpOld, *dpNew;

  if (DEBUG) printf(YELLOW "Enter unlink.\n" RESET);

  if (strlen(pathname) == 0)
  {
    printf(RED"No file specified for unlink.\n"RESET);
    return 0;
  }

  dev = running->cwd->dev;
  strcpy(file, pathname);
  strcpy(fileCopy, file);

  ino = getino(dev, file);

  if (!ino)
  {
    printf(RED"Can't find file.\n"RESET);
    return 0;
  }

  if (isDir(ino))
  {
    printf(RED"%s is a directory.\n"RESET, file);
    return 0;
  }

  mp = iget(dev, ino);
  mp->INODE.i_links_count--;
  mp->dirty = 1;

  // I think links count is inaccurate
  if (mp->INODE.i_links_count == 0) truncate(mp);

  strcpy(dirname, mydirname(file));
  strcpy(basename, mybasename(file));

  if (DEBUG) printf(YELLOW "dirname: %s, basename: %s\n" RESET, dirname, basename);

  if (strcmp(dirname, ".") == 0 || strcmp(dirname, "/") == 0)
  {
    if (dirname[0] == "/") pino = root->ino;
    else pino = running->cwd->ino;
  }
  else pino = getino(dev, file);

  pmp = iget(dev, pino);

  rmchild(pmp, basename);

  mp->dirty = 1;
  pmp->dirty = 1;

  iput(pmp);
  iput(mp);

  return 1;
}

int symlink()
{
  MINODE *mp, *nmp;
  char oldFile[256], newFile[256], oldCopy[256], newCopy[256], *cpOld, *cpNew, buff[BLOCK_SIZE], oldName[256], newName[256];
  int ino, dev, nino, i, found;
  DIR *dpOld, *dpNew;

  if (DEBUG) printf(YELLOW "Enter symlink.\n" RESET);

  dev = running->cwd->dev;
 
  if (strlen(pathname) == 0 || strlen(parameter) == 0)
  {
    printf(RED"Please provide source and destination filenames.\n"RESET);
    return 0;
  }

  if (DEBUG) printf(YELLOW"source: %s, dest: %s\n"RESET,pathname, parameter);
  strcpy(oldFile, pathname);
  strcpy(newFile, parameter);

  ino = getino(dev, oldFile);
  mp = iget(dev, ino);

  if (!isReg(ino) && !isDir(ino))
  {
    printf(RED"%s is not a file or directory.\n"RESET, oldFile);
    return 0;
  }

  iput(mp);

  strcpy(pathname, newFile);

  creat_file();

  nino = getino(dev, newFile);
  nmp = iget(dev, nino);
  nmp->INODE.i_mode = SYM_MODE;
  nmp->INODE.i_size = strlen(oldFile);

  // Copy name into iblocks area?
  memcpy(nmp->INODE.i_block, oldFile, strlen(oldFile));

  iput(nmp);
}

/************************************************************\
                      LEVEL 2 FUNCTIONS
\************************************************************/

int open_file()
{
  if (DEBUG) printf(YELLOW "Enter open_file.\n" RESET);
}

int close_file()
{
  if (DEBUG) printf(YELLOW "Enter close_fil.\n" RESET);
}

int read_file()
{
  if (DEBUG) printf(YELLOW "Enter read_file.\n" RESET);
}

int write_file()
{
  if (DEBUG) printf(YELLOW "Enter write_file.\n" RESET);
}

int cat_file()
{
  if (DEBUG) printf(YELLOW "Enter cat_file.\n" RESET);
}

int cp_file()
{
  if (DEBUG) printf(YELLOW "Enter cp_file.\n" RESET);
}

int mv_file()
{
  if (DEBUG) printf(YELLOW "Enter mv_file.\n" RESET);
}

int lseek_file()
{
  if (DEBUG) printf(YELLOW "Enter lseek_file.\n" RESET);
}

/************************************************************\
                      LEVEL 3 FUNCTIONS
\************************************************************/

int mount()
{
  if (DEBUG) printf(YELLOW "Enter mount.\n" RESET);
}

int umount()
{
  if (DEBUG) printf(YELLOW "Enter umount.\n" RESET);
}

/************************************************************\
                      UTILITY FUNCTIONS
\************************************************************/

int enter_name(MINODE *pip, int myino, char *myname)
{
  int i, remain, firstFreeBlock, newBlock;
  INODE *ip = &(pip->INODE);
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  for (i = 0; i < 12; i++)                          // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] == 0) break;

    get_block(running->cwd->dev, ip->i_block[i], buf);
    cp = (char *)buf;
    dp = (DIR *)cp;

    while (cp + dp->rec_len < (buf + BLOCK_SIZE))   // Move to the last entry in the block
    {
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    remain = dp->rec_len - (4 * (8 + dp->name_len + 3) / 4);

    if (remain >= (4 * (8 + strlen(myname) + 3) / 4))
    {
      dp->rec_len = (4 * (8 + dp->name_len + 3) / 4);

      cp += dp->rec_len;
      dp = (DIR *)cp;

      dp->inode = myino;
      dp->rec_len = remain;
      dp->name_len = strlen(myname);
      strncpy(dp->name, myname, strlen(myname));

      put_block(pip->dev, ip->i_block[i], buf);

      return 1;
    }
  }

  newBlock = balloc(running->cwd->dev);
  ip->i_block[i] = newBlock;
  ip->i_size += BLOCK_SIZE;

  // Insert new entry
  get_block(running->cwd->dev, ip->i_block[i], buf);
  cp = buf;
  dp = (DIR *)cp;

  dp->inode = myino;
  dp->rec_len = BLOCK_SIZE;
  dp->name_len = strlen(myname);
  dp->file_type = EXT2_FT_DIR;
  strncpy(dp->name, myname, strlen(myname));

  put_block(pip->dev, ip->i_block[i], buf);

  return 1;
}

int enter_filename(MINODE *pip, int myino, char *myname) // Same as enter_name but enters file_type as file instead of dir
{
  int i, remain, firstFreeBlock, newBlock;
  INODE *ip = &(pip->INODE);
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  for (i = 0; i < 12; i++)                               // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] == 0) break;

    get_block(running->cwd->dev, ip->i_block[i], buf);
    cp = (char *)buf;
    dp = (DIR *)cp;

    while (cp + dp->rec_len < (buf + BLOCK_SIZE))       // Move to the last entry in the block
    {
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    remain = dp->rec_len - (4 * (8 + dp->name_len + 3) / 4);

    if (remain >= (4 * (8 + strlen(myname) + 3) / 4))
    {
      dp->rec_len = (4 * (8 + dp->name_len + 3) / 4);

      cp += dp->rec_len;
      dp = (DIR *)cp;

      dp->inode = myino;
      dp->rec_len = remain;
      dp->name_len = strlen(myname);
      strncpy(dp->name, myname, strlen(myname));

      put_block(pip->dev, ip->i_block[i], buf);

      return 1;
    }
  }

  newBlock = balloc(running->cwd->dev);
  ip->i_block[i] = newBlock;
  ip->i_size += BLOCK_SIZE;

  // Insert new entry
  get_block(running->cwd->dev, ip->i_block[i], buf);
  cp = buf;
  dp = (DIR *)cp;

  dp->inode = myino;
  dp->rec_len = BLOCK_SIZE;
  dp->name_len = strlen(myname);
  dp->file_type = EXT2_FT_REG_FILE;
  strncpy(dp->name, myname, strlen(myname));

  put_block(pip->dev, ip->i_block[i], buf);

  return 1;
}

int truncate (MINODE *mp)
{
  int i;
  INODE *ip;

  for (i = 0; i < 12; i++)
  {
    ip = &mp->INODE;

    if (ip->i_block[i] == 0) break;                     // Assuming there are no gaps in the blocks

    bdealloc(mp->dev, ip->i_block[i]);
    mp->INODE.i_block[i] = 0;
    mp->dirty = 1;
    iput(mp);
    mp->refCount++;
  }

  return 1;
}

int menu()
{  
  printf(RED"\n/*"RESET"*********************************************************"RED"*\\\n"RESET); 
  printf("                       Accepted Commands                      \n\n");
  printf("\tmkdir\trmdir\tls\tcd\tpwd\tcreat\n");
  printf("\tlink\tunlink\tsymlink\tstat\tchmod\ttouch\n\n");
  printf(RED"\\*"RESET"*********************************************************"RED"*/\n"RESET);

  return 1;
}

int quit()
{
  int i;
  MINODE *mp;

  if (DEBUG) printf(YELLOW"Enter quit.\n"RESET);

  // iput all dirty nodes
  for (i = 0; i < NMINODES; i++)
  {
    mp = &minode[i];

    if (mp->dirty)
      iput(mp);
  }

  printf("Exiting Program.\n");
  exit(1);
}

int pfd()
{
  if (DEBUG) printf(YELLOW "Enter pfd.\n" RESET);
}

int rewind_file()
{
  if (DEBUG) printf(YELLOW "Enter rewind_file.\n" RESET);
}

int pm()
{
  if (DEBUG) printf(YELLOW "Enter pm.\n" RESET);
}

int access_file()
{  
  if (DEBUG) printf(YELLOW "Enter access_file.\n" RESET);

}

int cs()
{
  if (DEBUG) printf(YELLOW "Enter cs.\n" RESET);
}

int do_fork()
{
  if (DEBUG) printf(YELLOW "Enter do_fork.\n" RESET);
}

int do_ps()
{
  if (DEBUG) printf(YELLOW "Enter do_ps.\n" RESET);
}

int do_kill()
{
  if (DEBUG) printf(YELLOW "Enter do_kill.\n" RESET);
}

int sync()
{
  if (DEBUG) printf(YELLOW "Enter sync.\n" RESET);
}