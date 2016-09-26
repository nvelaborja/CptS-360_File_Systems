/*************************************      gd.c      ***************************************\
*	Written By: Nathan VelaBorja
*	Date: March 30, 2016
*	Assignment: CptS 360 Lab 6
*	Description: 
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned int u32;

typedef struct ext2_group_desc 	GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode	INODE;
typedef struct ext2_dir_entry_2	DIR;

GD *gp;
SUPER *sp;
INODE *ip;
DIR *dp;

#define BLKSIZE 1024

/******  GD struct  ********
struct ext2_group_desc
{
  u32  bg_block_bitmap;          // Bmap block number
  u32  bg_inode_bitmap;          // Imap block number
  u32  bg_inode_table;           // Inodes begin block number
  u16  bg_free_blocks_count;     // THESE are OBVIOUS
  u16  bg_free_inodes_count;
  u16  bg_used_dirs_count;        

  u16  bg_pad;                   // ignore these 
  u32  bg_reserved[3];
};
***************************/

char buf[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read (fd, buf, BLKSIZE);
}

gd()
{
	// read GD block
	get_block(fd, 2, buf);
	gp = (GD *)buf;

	printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap); 
	printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap); 
	printf("bg_inode_table = %d\n", gp->bg_inode_table); 
	printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count); 
	printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count); 
	printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count); 

}

char *disk = "mydisk";			// default disk incase no disk was entered as parameter

main(int argc, char *argv[])
{
	if (argc > 1)
		disk = argv[1];
	fd = open(disk, O_RDONLY);
	
	if (fd < 0)
	{
		printf("Disk open failure.\n");
		exit(1);
	}

	gd();
}



