/**********************************      showblock.c      ****************************************\
*       Written By: Nathan VelaBorja
*       Date: March 30, 2016
*       Assignment: CptS 360 Lab 6
*       Description: 
**************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define BLKSIZE 1024

// ********** STRUCT SIMPLIFICATION ********** // 

typedef struct ext2_group_desc	GD;
typedef struct ext2_super_block	SB;
typedef struct ext2_inode	INODE;
typedef struct ext2_dir_entry_2	DIRECTORY;

// ***********  GLOBAL VARIABLES   *********** // 

char *path;
char *device;

int fd;
int iblock;

char *tokens[10];				// Path names can have a max of ten nodes
int numTokens;

// ********** 	MAIN	 ********** // 

int main (int argc, char *argv[])
{
	char buf[BLKSIZE], buf2[BLKSIZE], *cp, temp[256], *token;
	int index = 0, ino, bno;	
	GD	*gp;
	SB	*sp;
	INODE	*ip;
	DIRECTORY *dp;


	if (argc < 3)
	{
		printf("Please provide device and path.\n");
		exit(1);
	}

	device = argv[1];
	path = argv[2];

	// Open the device for READ
	fd = open(device, O_RDONLY);

	if (fd < 0)
	{
		printf("Failed to open %s.\n", device);
		exit(1);
	}

	// Read in Superblock, verify it's an ext2 fs
	printf("Reading Super Block.\n");

	get_block(fd, 1, buf);
	sp = (SB *)buf;

	if (sp->s_magic != 0xEF53)
	{
		printf("Sorry, %s is not an EXT2 File System.\n", device);
		exit(1);
	}

	// Read in group block, get InodesBeginBlock
	printf("Reading Group Descriptor Block.\n");

	get_block(fd, 2, buf);
	gp = (GD *)buf;
	cp = buf;

	iblock = gp->bg_inode_table;

	printf("\tINodeBeginBlock: %d\n\n", iblock);

	// Read InodesBeginBlock to get root dir
	printf("Reading INodeBeginBlock.\n");

	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;		// root dir

	printf("\t&ip of INodeBeginBlock: %d\n\n", ip);
	
	// Tokenize pathname
	
	tokenizePath(path);	

	// Search for token
	printf("Begin token searching.\n");
	printf("\tbuf: %x\n", buf);

	for (index = 0; index < numTokens; index++)
	{
		printf("Loop iteration %d:\n", index + 1);

		ino = 0;
		bno = 0;

		ino = search(ip, tokens[index]);

		printf("\tino from search: %x\n", buf);

		if (!ino)
		{
			printf("Search failure. Exiting program.\n");
			return 0;
		}

		// At this point, a match must have been found for the current token, move to that inode
		
		// Use mailman's algorithm to find inod

		bno = (ino - 1) / 8 + iblock;
		ino = (ino - 1) % 8;

		printf("\tino after mma: %d\n", ino);
		printf("\tbno after mma: %d\n", bno);

		get_block(fd, bno, buf2);
		ip = (INODE *)buf2 + ino;	

		printf("\tbuf after mma: %x\n", buf);
		printf("\t&ip after mma: %d\n", ip);
		printf("\tInode mode: %d\n\n", ip->i_mode);

		printf("Checking to see if inode is a directory.\n");

		if (index != numTokens - 1 && !S_ISDIR(ip->i_mode))		// If we are expecting a directory and it isn't one, exit
		{
			printf("\t%s is not a directory. Aborting.\n", tokens[index]);
			return 0;
		}		
	}
	// INODE Found! Now just print it's info

	printf("\nNode Found!\n");
	printf("\ti_mode : \t%d\n", ip->i_mode);
	printf("\ti_uid : \t%d\n", ip->i_uid);
	printf("\ti_size : \t%d\n", ip->i_size);
	printf("\ti_gid : \t%d\n", ip->i_gid);
	printf("\ti_links_count : \t%d\n\n", ip->i_links_count);

	return 0;
}

// **********	  UTILITY FUNCTIONS	********** // 

int search(INODE *ip, char *name)
{
	int i, inode;
	char buff[BLKSIZE];
	INODE *pip = ip;

	printf("Enter search(). Looking for: %s\n", name);
	printf("\tGiven &ip: %d\n\tGiven name: %s\n", ip, name);
	printf("\tInitial buf: %x\n", buff);

    for (i = 0; i < 12; i++)									// Assume only 12 direct blocks
	{
		char *cp, dirname[256];
		DIRECTORY *dp;

		get_block(fd, pip->i_block[i], buff);

		printf("\tBuf from ip->i_block[%d]: %x\n", i, buff);

		cp = buff;
		dp = (DIRECTORY *)cp;

		printf("\tdp->inode: %d\n", dp->inode);

		printf("\tBegining Scan...\n");

		while (cp < buff + BLKSIZE)
		{
			strncpy(dirname, dp->name, dp->name_len);
			dirname[dp->name_len] = 0;

			if (!dp->inode)	return 0;

			printf("\t\tCurrent dir: %s (%d)\n", dp->name, dp->inode);

			if (strcmp(dirname, name) == 0)						// Match found!
			{
				printf("\t\tFound %s! It's ino is %d.\n\n", name, dp->inode);
				return dp->inode;
			}

			cp += dp->rec_len;									// If we didn't find a match, move on to the next one
			dp = (DIRECTORY *)cp;
		}

		printf("\tCouldn't find %s. Are you sure it exists?\n\n", name);

		return 0;
  	}

	return inode;
}

int get_block(int fd, int blk, char buffer[])
{
	printf("Enter get_block().\n\tGiven fd: %d\n\tGiven blk: %d\n\tGiven buf: %x\n", fd, blk, buffer);

	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buffer, BLKSIZE);

	printf("\tbuf after lseek/read: %x\n\n", buffer);

	return 1;
}

int tokenizePath(char *path)
{
	char *token;
	int index = 1;
	
	printf("Enter tokenizePath().\n\tGiven Path: %s\n", path);

	numTokens = 0;

	if (path[0] == '/')				// If path starts with a '/', remove it to simplify tokens
		token++;

	token = strtok(path, "/");
	tokens[0] = (char *)malloc(strlen(token));		// Allocate and copy first token to global tokens array
	strcpy(tokens[0], token);

	printf("\tToken 1: %s\n", token);

	numTokens++;

	while (token = strtok(NULL, "/"))		// Allocate and copy the rest of the tokens
	{
		printf("\tToken %d: %s\n", index + 1, token);

		tokens[index] = (char *)malloc(strlen(token));
		strcpy(tokens[index], token);
		numTokens++;
		index++;
	}

	printf("\n");

	return 1;
}



