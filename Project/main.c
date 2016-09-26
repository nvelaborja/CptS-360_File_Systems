#include "type.h"
#include "util.c"
#include "commands.c"

/**********************  prototypes ***************************/

int mountroot();
int init();
int findCmd(char *command);
int displayRunning();

/************************ globals *****************************/
MINODE *root; 
char pathname[128], parameter[128], *name[128], cwdname[128], parentPth[128];
//char names[128][256];

int  nnames;
char *rootdev = "mydisk", *slash = "/", *dot = ".";
int iblock;
int dev;

MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];

typedef void (*func)();

char names[64][64];
int fd, imap, bmap, numTokens, isPath, isParameter;
int ninodes, nblocks, nfreeInodes, nfreeBlocks, numTokens;
char *cmd[] = {"mkdir", "cd", "pwd", "ls", "mount", "umount", "creat", "rmdir", 
      "rm", "open", "close", "read", "write", "cat", "cp", "mv", "pfd", "lseek", 
      "rewind", "stat", "pm", "menu", "access", "chmod", "chown", "cs", "fork", 
      "ps", "kill", "quit", "touch", "sync", "link", "unlink", "symlink"}; // List of commands accepted by this simulation 

func fpointers[] = {make_dir, change_dir, pwd, list_dir, mount, umount, creat_file, rmdir, 
      rm_file, open_file, close_file, read_file, write_file, cat_file, cp_file, mv_file, pfd, lseek_file, 
      rewind_file, mystat, pm, menu, access_file, chmod_file, chown_file, cs, do_fork, 
      do_ps, do_kill, quit, do_touch, sync, link, unlink, symlink};

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

MOUNT *getmountp();

int DEBUG=0;
int nproc=0;

int mountroot() 
{
  int i, ino, fd, dev;
  MOUNT *mp;
  SUPER *sp;
  MINODE *ip;

  char line[64], buf[BLOCK_SIZE], *rootdev;
  int ninodes, nblocks, ifree, bfree;

  printf("enter rootdev name (RETURN for mydisk) : ");
  gets(line);

  rootdev = "mydisk";

  if (line[0] != 0)
     rootdev = line;

  dev = open(rootdev, O_RDWR);
  if (dev < 0){
     printf("panic : can't open root device\n");
     exit(1);
  }

  /* get super block of rootdev */
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* check magic number */
  printf("SUPER magic=0x%x  ", sp->s_magic);
  if (sp->s_magic != SUPER_MAGIC){
     printf("super magic=%x : %s is not a valid Ext2 filesys\n", sp->s_magic, rootdev);
     exit(0);
  }

  mp = &mounttab[0];      /* use mounttab[0] */

  /* copy super block info to mounttab[0] */
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;

  bfree = sp->s_free_blocks_count;
  ifree = sp->s_free_inodes_count;

  get_block(dev, 2, buf);
  gp = (GD *)buf;

  mp->dev = dev;         
  mp->busy = BUSY;

  mp->bmap = gp->bg_block_bitmap;
  mp->imap = gp->bg_inode_bitmap;
  mp->iblock = gp->bg_inode_table;

  strcpy(mp->name, rootdev);
  strcpy(mp->mount_name, "/");


  printf("bmap=%d  ",   gp->bg_block_bitmap);
  printf("imap=%d  ",   gp->bg_inode_bitmap);
  printf("iblock=%d\n", gp->bg_inode_table);  
  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblock = gp->bg_inode_table;

  root = iget(dev, 2);          /* get root inode */
  mp->mounted_inode = root;
  root->mountptr = mp;

  printf("mount : %s  mounted on / \n", rootdev);
  printf("nblocks=%d  bfree=%d   ninodes=%d  ifree=%d\n", nblocks, bfree, ninodes, ifree);
  printf("Root Info: dev - %d   ino- %d   refC - %d   dirty - %d  mounted - %d   INODE.size - %d\n", root->dev, root->ino, root->refCount, root->dirty, root->mounted, root->INODE.i_size);


  return(0);
} 

int init()
{
  int i, j;
  PROC *p;

  for (i=0; i<NMINODES; i++)
    minode[i].refCount = 0;

  for (i=0; i<NMOUNT; i++)
    mounttab[i].busy = 0;

  for (i=0; i<NPROC; i++)
  {
    proc[i].status = FREE;
    for (j=0; j<NFD; j++)
      proc[i].fd[j] = 0;
    proc[i].next = &proc[i+1];
  }

  for (i=0; i<NOFT; i++)
    oft[i].refCount = 0;

  printf("mounting root\n");
  mountroot();
  printf("mounted root\n");
  printf("creating P0, P1\n");

  p = running = &proc[0]; p->status = BUSY; p->uid = 0;  p->pid = p->ppid = p->gid = 0;
  p->parent = p->sibling = p; p->child = 0; p->cwd = root; p->cwd->refCount++;

  p = &proc[1]; p->next = &proc[0]; p->status = BUSY; p->uid = 2;  p->pid = 1; 
  p->ppid = p->gid = 0; p->cwd = root; p->cwd->refCount++;
  
  nproc = 2;

  return 1;
}

int main(int argc, char *argv[ ]) 
{
  int i,cmd; 
  char line[128], cname[64];

  if (argc>1){
    if (strcmp(argv[1], "-d")==0)
        DEBUG = 1;
  }

  printf(RED "*************************************************************************************************\n");
  printf(RESET "*************************************************************************************************\n");
  printf(BLUE "*************************************************************************************************\n" RESET);

  init();

  while(1){
      if (DEBUG) printf(YELLOW"P%d running: \n"RESET, running->pid);
    
      /* set the strings to 0 */
      memset(pathname, 0, 64);
      memset(parameter,0, 64);
      isPath = 0;
      isParameter = 0;

      displayRunning();
      displayMINODES();

      if (DEBUG) printf(YELLOW "Root ino: %d\n" RESET, root->ino);
      if (DEBUG) printf(YELLOW "CWD  ino: %d\n" RESET, running->cwd->ino);

      printf(CYAN "\ninput command : " RESET);
      //pwd();
      printf(CYAN "> "RESET);
      gets(line);
      if (line[0]==0) continue;

      sscanf(line, "%s %s %64c", cname, pathname, parameter);
      if (DEBUG) printf(YELLOW"cname: %s, pathname: %s, parameter: %s\n"RESET, cname, pathname, parameter);

      if (strlen(pathname) != 0) isPath = 1;
      if (strlen(parameter) != 0) isParameter = 1;

      cmd = findCmd(cname);

      if (cmd == -1) 
      {
        printf(RED"Invalid Command.\n"RESET);
        continue;
      }
      
      fpointers[cmd]();
  }

  return 1;
}

int findCmd(char *command)
{
  int i = 0;
  
  while (cmd[i])
  {
    if (strcmp(command, cmd[i]) == 0)
      return i;
    i++;
  }

  return -1;
}

int displayRunning()
{
  if (!DEBUG) return;

  printf(YELLOW ": RUNNING PROCESS :\n");
  printf("\tuid: %d\n", running->uid);
  printf("\tpid: %d\n", running->pid);
  printf("\tppid: %d\n", running->ppid);
  printf("\tgid: %d\n", running->gid);
  printf("\tstatus: %d\n", running->status);
  printf("\tcwd: dev - %d  ino - %d\n", running->cwd->dev, running->cwd->ino);
  if (running->next) printf("\t Has *next.\n");
  if (running->child) printf("\t Has *child.\n");
  if (running->parent) printf("\t Has *parent.\n");
  if (running->sibling) printf("\t Has *sibling.\n");
  printf("\n" RESET);

  return 1;
}