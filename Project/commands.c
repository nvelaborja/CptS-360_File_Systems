extern char *t1;
extern char *t2;


// Function Level: 1
int make_dir()
{
  MINODE *mip, *pip;
  char parent[256], child[256];
  int pino;
  
  printf("Enter make_dir.\n");

  // We need to create a dir at the location specified
  if (pathname[0] == '/') 
  {
    mip = root;
    dev = root->dev;
  }
  else
  {
    mip = running->cwd;
    dev = running->cwd->dev;
  }

  parent = mydirname(pathname);
  child = mybasename(pathname);

  pino = getino(&dev, parent);
  pip = iget(dev, pino);

  if (!isDir(pino)) return 0;
  if (search(pip, child)) return 0;

  mymkdir(pip, child);

  pip->INODE.i_links_count += 1;

  // TODO: Touch parent's atime

  pip->dirty = 1;

  iput(pip);

  return 1;
}

int mymkdir(MINODE *pip, char *name)
{
  int ino, bno, i;
  MINODE *mip;
  INODE *ip;
  char buf[BLOCK_SIZE], *cp;
  DIR cur, prev;

  ino = ialloc(dev);
  bno = balloc(dev);

  mip = iget(dev, ino);
  ip = &mip->INODE;

  ip->i_mode = DIR_MODE;
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = BLOCK_SIZE;
  ip->i_links_count = 2;
  ip->i_atime = i_ctime = i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = bno;
  for (i = 1; i < 15; i++)
    ip->i_block[i] = 0;

  mip->dirty = 1;

  iput(mip);

  // Create . and .. dir entries
  cur.inode = mip->ino;
  cur.name_len = 1;
  cur.file_type = DIR_MODE;
  cur.name[1] = '.';
  cur.rec_len = 4 * ((8 + cur.name_len + 3) / 4);

  prev.inode = pip->ino;
  prev.name_len = 2;
  prev.file_type = DIR_MODE;
  prev.name[1] = '.';
  prev.name[2] = '.';
  prev.rec_len = BLOCK_SIZE - cur.rec_len;

  // Write . and .. to buf
  cp = &(char *)cur;
  for (i = 0; i < cur.rec_len; i++)
  {
    buf[i] = cp[i];
  }
  cp = &(char*)prev;
  for (i = 0; i < prev.rec_len; i++)
  {
    buf[i + cur.rec_len] = cp[i];
  }

  // Write buf to the disk block bno
  write(bno, buf, BLOCK_SIZE);

  // Enter name ENTRY into parent's directory 
  enter_name(pip, ino, name);

  return 1;
}

int enter_name(MINODE *pip, int myino, char *myname)
{
  int i, offset, remain;
  INODE *ip = &(pip->INODE);
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  for (i = 0; i < 12; i++)            // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] == 0) break;

    get_block(dev, ip->i_block, buf);
    cp = (char *)buf;
    dp = (DIR *)cp;

    offset = 0;

    while (offset + dp->rec_len < BLOCK_SIZE)   // Move to the last entry in the block
    {
      offset += dp->rec_len;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    remain = dp->rec_len - (4 * (8 + dp->rec_len + 3) / 4);

    if (remain >= (4 * (8 + dp->rec_len + 3) / 4))
    {
      // TODO: Shrink dp's rec_len to it's ideal size, insert new entry at the end?
    }
    else
    {
      // No more space in this block, allocate new one, insert new entry as the first with rec_len = blocksize
    }

    // Write block to disk
  }

}

// Function Level: 1
int change_dir()
{
  MINODE *mp;
  INODE *ip;
  int ino;

  printf("Enter change_dir.\n");

  // We need to change cwd to root if no pathname is provided
  if (strlen(pathname) == 0)
  {
    running->cwd = root;
    return 1;
  }

  // Otherwise change cwd to whatever path provided
  ino = getino(running->cwd->dev, pathname);
  mp = iget(running->cwd->dev, ino);
  running->cwd = mp;

  return 1;
}

// Function Level: 1
int pwd(MINODE *cwd)
{
  printf("Enter pwd.\n");

  // We need to start at cwd and go backwards through .. dirs until we hit the root dir to build path name
  rpwd(cwd);
}

int rpwd(MINODE *node)
{
  MINODE *mpp;
  INODE *ip;
  DIR *dp;
  char *cp, buf[BLOCK_SIZE], name[256];

  if (!node) return 0;            // If we have reached a null node for some reason, return

  // Get the dir's parent (..)
  get_block(node->dev, node->INODE.i_block[1], buf);
  cp = buf;
  dp = (DIR *)cp;

  strncpy(name, dp->name, dp->name_len);
  name[strlen(name)] = 0;

  mpp = iget(node->dev, dp->inode);

  findmyname(mpp, node->ino, name);

  rpwd(mpp);


  printf("%s/", name);

  return 1;
}

// Function Level: 1
int list_dir()
{
  int inode, i, j, dev;
  char name[MAXTOKENSIZE * 2], cwd[MAXTOKENSIZE * 2], buf[BLOCK_SIZE], *cp, ftime[64];
  MINODE *mp, *tmp;
  DIR *dp;
  INODE *ip;

  printf("Enter list_dir.\n");

  dev = running->cwd->dev;

  if (strlen(pathname) == 0)
  {
    strcpy(pathname, "./");
  }
  /* Format Example:
  drwxr-xr-x 2 nathan nathan 4096 Mar 30 20:57 Desktop
  drwxr-xr-x 4 nathan nathan 4096 Feb  9 11:21 Documents
  drwxr-xr-x 5 nathan nathan 4096 Apr  9 21:15 Downloads */

  inode = getino(&(dev), pathname);      // Get the inode 
  mp = iget(dev, inode);              // Get the MINODE so we can access it's INODE struct

  // File
  if (S_ISREG(mp->INODE.i_mode))            // If the specified inode is a file, just print the file's info
  {
    if ((mp->INODE.i_mode & 0x8000) == 0x8000) printf("%c", '-');
    if ((mp->INODE.i_mode & 0x4000) == 0x4000) printf("%c", 'd');
    if ((mp->INODE.i_mode & 0xA000) == 0xA000) printf("%c", 'l');

    for (i = 8; i > -1; i--)
    {
        if (mp->INODE.i_mode & (1<<i)) printf("%c", t1[i]);
        else printf("%c", t2[i]);
    }

    printf("\t%4d ", mp->INODE.i_links_count);
    printf("%4d ", mp->INODE.i_gid);
    printf("%4d ", mp->INODE.i_uid);
    printf("%8d ", mp->INODE.i_size);

    strcpy(ftime, ctime(mp->INODE.i_ctime));
    ftime[strlen(ftime) - 1] = 0;
    printf("%s  ", ftime);
    printf("%s\n", basename(name));

    return 1;
  }

  // Directory
  ip = &(mp->INODE);
  for (i = 0; i < 12; i++)                  // Assuming only 12 direct blocks
  {
    if (ip->i_block[i] = 0) return;         // Return if we hit the end of entries

    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLOCK_SIZE)
    {
      tmp = iget(dev, dp->inode);

      if ((tmp->INODE.i_mode & 0x8000) == 0x8000) printf("%c", '-');
      if ((tmp->INODE.i_mode & 0x4000) == 0x4000) printf("%c", 'd');
      if ((tmp->INODE.i_mode & 0xA000) == 0xA000) printf("%c", 'l');

      for (j = 8; j > -1; j--)
      {
        if (tmp->INODE.i_mode & (1<<i)) printf("%c", t1[i]);
        else printf("%c", t2[i]);
      }

      printf("\t%4d ", tmp->INODE.i_links_count);
      printf("%4d ", tmp->INODE.i_gid);
      printf("%4d ", tmp->INODE.i_uid);
      printf("%8d ", tmp->INODE.i_size);

      strcpy(ftime, ctime(tmp->INODE.i_ctime));
      ftime[strlen(ftime) - 1] = 0;
      printf("%s  ", ftime);
      printf("%s\n", basename(name));

      // Move cp/dp to the next entry
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }

  //running->cwd->INODE;
  //printf("mode: %d\n", root->INODE.i_mode);
}

int mount()
{
  printf("Enter mount.\n");
}

int umount(char *pathname)
{
  printf("Enter umount.\n");
}

// Function Level: 1
int creat_file()
{
  printf("Enter creat_file.\n");
}

// Function Level: 1
int rmdir()
{
  printf("Enter rmdir.\n");
}

// Function Level: 1
int rm_file()
{
  printf("Enter rm_file.\n");
}

int open_file()
{
  printf("Enter open_file.\n");
}

int close_file()
{
  printf("Enter close_file.\n");
}

int read_file()
{
  printf("Enter read_file.\n");
}

int write_file()
{
  printf("Enter write_file.\n");
}

int cat_file()
{
  printf("Enter cat_file.\n");
}

int cp_file()
{
  printf("Enter cp_file.\n");
}

int mv_file()
{
  printf("Enter mv_file.\n");
}

int pfd()
{
  printf("Enter pfd.\n");
}

int lseek_file()
{
  printf("Enter lseek_file.\n");
}

int rewind_file()
{
  printf("Enter rewind_file.\n");
}

// Function Level: 1
int mystat()
{
  printf("Enter mystat.\n");
}

int pm()
{
  printf("Enter pm.\n");
}

// Function Level: 1
int menu()
{  
  printf("Enter menu.\n");

}

int access_file()
{  
  printf("Enter access_file.\n");

}

// Function Level: 1
int chmod_file()
{
  printf("Enter chmod_file.\n");
}

int chown_file()
{
  printf("Enter chown_file.\n");
}

int cs()
{
  printf("Enter cs.\n");
}

int do_fork()
{
  printf("Enter do_fork.\n");
}

int do_ps()
{
  printf("Enter do_ps.\n");
}

int do_kill()
{
  printf("Enter do_kill.\n");
}

// Function Level: 1
int quit()
{
  int i;
  MINODE *mp;

  printf("Enter quit.\n");

  // iput all dirty nodes

  for (i = 0; i < NMINODES; i++)
  {
    mp = minode[i];

    if (mp->dirty)
    {
      iput(mp);
    }
  }

  printf("Exiting Program.\n");
  exit(1);
}

// Function Level: 1
int do_touch()
{
  printf("Enter do_touch.\n");
}

int sync()
{
  printf("Enter sync.\n");
}

// Function Level: 1
int link()
{
  printf("Enter link.\n");
}

// Function Level: 1
int unlink()
{
  printf("Enter unlink.\n");
}

// Function Level: 1
int symlink()
{
  printf("Enter symlink.\n");
}

