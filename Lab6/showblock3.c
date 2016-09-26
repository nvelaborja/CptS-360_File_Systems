#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>
#include <sys/stat.h>
#define BLKSIZE 1024
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs
int fd, iblock;
GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 
char name[10][64];
char *line;
int dirnums;
u32 InodesBeginBlock;
u32 INODES_PER_BLOCK;
char *disk = "mydisk2";
char buf[BLKSIZE];



int parseLine()
{
	dirnums=0;
	int i = 1;
	char *token;
	char linecpy[128];
	
	if (strcmp(line, "/") == 0)
	{
		showroot();
	 return;
}
	strcpy(linecpy, line);

	token = strtok (linecpy, "/");
	dirnums++;
	
	//line[0] = (char *)malloc(strlen(line));
	strcpy(name[0], token);

	while (token = strtok(0, "/"))
	{
		//args[i] = (char *)malloc(strlen(token));
		strcpy(name[i], token);
		i++;
		dirnums++;
	}	
}

inodeBegin()
{
  
  printf("Bmap:%d imap:%d inodesBgin:%d freeblocks:%d freeInodes:%d usedDirs:%d\n",
	 gp->bg_block_bitmap,
	 gp->bg_inode_bitmap,
	 gp->bg_inode_table,
	 gp->bg_free_blocks_count,
	 gp->bg_free_inodes_count,
	 gp->bg_used_dirs_count);
   
  InodesBeginBlock = gp->bg_inode_table;
  printf("inode_block=%d\n", InodesBeginBlock);

  // get inode start block     
  get_block(fd, InodesBeginBlock, buf);

  ip = (INODE *)buf + 1;         // ip points at 2nd INODE
  
  printf("mode=%4x ", ip->i_mode);
  printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
  printf("size=%d\n", ip->i_size);
  printf("time=%s", ctime(&ip->i_ctime));
  printf("link=%d\n", ip->i_links_count);
  printf("i_block[0]=%d\n", ip->i_block[0]);
}
super()
{
  // read SUPER block
  get_block(fd, 1, buf);  
  sp = (SUPER *)buf;
	
  // check for EXT2 magic number:

  //printf("s_magic = %x\n", sp->s_magic);
  if (sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    exit(1);
  }

  printf("SUPER BLOCK INFO\n");
  printf("inodes_count = %d\n", sp->s_inodes_count);
  printf("blocks_count = %d\n", sp->s_blocks_count);
  printf("free_inodes_count = %d\n", sp->s_free_inodes_count);
  printf("free_blocks_count = %d\n", sp->s_free_blocks_count);
  printf("first_data_blcok = %d\n", sp->s_first_data_block);
  printf("log_block_size = %d\n", sp->s_log_block_size);
  printf("blocks_per_group = %d\n", sp->s_blocks_per_group);
  printf("inodes_per_group = %d\n", sp->s_inodes_per_group);
  printf("mnt_count = %d\n", sp->s_mnt_count);
  printf("max_mnt_count = %d\n", sp->s_max_mnt_count);
  printf("magic = %x\n", sp->s_magic);
}

groupDesc()
{
  // read GROUP DESCRIPTOR block
  get_block(fd, 2, buf);  
  gp = (GD *)buf;

  //PRINT STUFF
	printf("Group INFO:\n");
  printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
  printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);

  printf("bg_inode_table = %d\n", gp->bg_inode_table);
  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);

  printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
	InodesBeginBlock = gp->bg_inode_table;
}

int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

u32 search(INODE *IP, char *name)
{
	//printf("enter search");
	int i = 0;
	while (i < 15)
	{
		char *cp; char temp[256];
		printf("i_block[i] = %d\n", i, IP->i_block[i]);
		get_block(fd, IP->i_block[i], buf);
		cp = buf;
		dp = (DIR *)buf;

		while(cp < buf + BLKSIZE){
		
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			if (dp->inode == 0)
			{return 0;}
			printf("%d  %d  %d  %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
			if (strcmp(temp, name) == 0)
			{return dp->inode;}
		           // move to the next DIR entry:
			cp += dp->rec_len;   // advance cp by rec_len BYTEs
			dp = (DIR *)cp;     // pull dp along to the next record
		}
		i++;
	}
	return 0;
}
showroot()
{// read SUPER block
	char *cp;
	char temp[256];
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;
	
	get_block(fd, 2, buf);
  gp = (GD *)buf;
	
	iblock = gp->bg_inode_table;
	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;         // ip points at 2nd INODE
	int i=0;
	printf("Direct Disk Blocks\n");
			while (1)
			{
				if (ip->i_block[i] == 0)
					break;
				if (i == 12)
					printf("Indirect Disk Blocks\n");
				if (i == 13)
					printf("Double Indirect Disk Blocks\n");
				printf("block[%d]: %d\n", i, ip->i_block[i]);
				i++;
			}
}
main(int argc, char *argv[])
{
	int Block, Inode;
	INODES_PER_BLOCK = BLKSIZE/sizeof(INODE);
	int i = 0;
  if (argc > 1)
    disk = argv[1];

  fd = open(disk, O_RDONLY);
  if (fd < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
	line = argv[2];
	//line = "/dir1";
	super();
	groupDesc();
	parseLine();
	if (dirnums == 0)
		return;
	printf("\nDirnums: %d\n", dirnums);
	for (i = 0; i < dirnums; i++)
	{
		printf("name[%d]:%s ", i, name[i]);
	}
	printf("\n");
	get_block(fd, InodesBeginBlock, buf);
	ip = (INODE *)buf + 1;         // ip points at 2nd INODE
	i = 0;
	while (i < dirnums)
	{
		u32 ino = search(ip, name[i]);
		if (ino == 0)
		{
				printf("cannot find %s\n", name[i]);
				return;
		}
		i++;
		printf("dirnum:%d\n", dirnums);
		Block = (ino-1)/INODES_PER_BLOCK + InodesBeginBlock;
		Inode = (ino-1)%INODES_PER_BLOCK;
		printf("block:%d inode:%d\n", Block, Inode);
		//get_block(fd, ino, buf);
		//ip = (INODE *)buf;
		get_block(fd, Block, buf);
		ip = (INODE *)buf + Inode;
		if (!S_ISDIR(ip->i_mode) && i < dirnums)
		{	
			printf("%s is not a dir.\n");
			return;		
		}
		if (i >= dirnums)
		{
			int j = 0;
			printf("Direct Disk Blocks\n");
			while (1)
			{
				if (ip->i_block[j] == 0)
					break;
				if (j == 12)
					printf("Indirect Disk Blocks\n");
				if (j == 13)
					printf("Double Indirect Disk Blocks\n");
				printf("block[%d]: %d\n", j, ip->i_block[j]);
				j++;
			}
		}
		
	}
	
}