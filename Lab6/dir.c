/**************************      dir.c     **************************************************\
*	Written By: Nathan VelaBorja
*	Date: March 30, 2016
*       Assignment: CptS 360 Lab 6
*       Description: 
*********************************************************************************************/

/*
struct ext2_dir_entry_2 {
	u32  inode;        // Inode number; count from 1, NOT from 0
	u16  rec_len;      // This entry length in bytes
	u8   name_len;     // Name length in bytes
	u8   file_type;    // for future use
	char name[EXT2_NAME_LEN];  // File name: 1-255 chars, no NULL byte
}; */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include<ext2fs/ext2_fs.h>

#define BLKSIZE 1024

typedef struct ext2_group_desc	GD;
typedef struct ext2_super_block	SUPER;
typedef struct ext2_inode	INODE;
typedef struct ext2_dir_entry_2	DIR;

GD	*gp;
SUPER	*sp;
INODE	*ip;
DIR	*dp;

int fd;
int iblock;

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

dir()
{
	char buf[BLKSIZE], *cp, temp[256];
	int index = 1;

	// read GD
	get_block(fd, 2, buf);
	gp = (GD *)buf;
	cp = buf;
	iblock = gp->bg_inode_table;	// Get inode start block#
	// Get inode start block
	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;		// I think this inode is the root dir

	printf("i_block[0]: %d\n", ip->i_block[0]);

	get_block(fd, ip->i_block[0], buf);	
	cp = (char *)buf;
	dp = (DIR *)buf;

	while (index < 5)		// I don't know how to properly cease this loop
	{	
		printf("%s ", dp->name);
		
		// Move cp to the next entry
		cp += dp->rec_len;
		dp = (DIR *)cp;
		index++;
	}
	printf("\n");

}

char *disk = "mydisk";				// Default disk

main (int argc, char *argv[])
{
	if (argc > 1) disk = argv[1];

	fd = open(disk, O_RDONLY);

	if (fd < 0)
	{
		printf("Open %s failure\n", disk);
		exit(1);
	}

	dir();
}

