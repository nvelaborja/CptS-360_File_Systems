/**********************************      bmap.c      ****************************************\
*	Written By: Nathan VelaBorja
*	Date: March 30, 2016
*       Assignment: CptS 360 Lab 6
*       Description: 
*********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef struct ext2_group_desc 	GD;
typedef struct ext2_super_block	SUPER;
typedef struct ext2_inode	INODE;
typedef struct ext2_dir_entry_2	DIR;

#define BLKSIZE 1024

GD	*gp;
SUPER	*sp;
INODE	*ip;
DIR	*dp;

char buf[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
	int i, j;
	
	i = bit / 8; j = bit % 8;
	if (buf[i] & (1 << j))
		return 1;
	return 0;
}

bmap()
{
	char buff[BLKSIZE];
	int bmap, ninodes;
	int i;

	// First, read SUPER block
	get_block (fd, 1, buf);
	sp = (SUPER *)buf;

	ninodes = sp->s_inodes_count;
	printf("ninodes = %d\n", ninodes);

	// Next, read GD block
	get_block (fd, 2, buf);
	gp = (GD *)buf;
	
	bmap = gp->bg_block_bitmap;
	printf("bmap = %d\n", bmap);

	// Next, read block_bitmap block
	get_block(fd, bmap, buf);

	for (i = 0; i < ninodes; i++)
	{
		(tst_bit(buf, i)) ? putchar('1') : putchar('0');
		if (i && (i % 8) == 0)
			printf(" ");
	}
	printf("\n");
}

char *disk = "mydisk";			// Default disk 

main (int argc, char *argv[])
{
	if (argc > 1)
		disk = argv[1];

	fd = open(disk, O_RDONLY);
	
	if (fd < 0)
	{
		printf("Open %s failure\n", disk);
		exit(1);

	}
	
	bmap();
}


