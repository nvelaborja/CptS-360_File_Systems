/**********************************      showblock.c      ****************************************\
*       Written By: Nathan VelaBorja
*       Date: March 30, 2016
*       Assignment: CptS 360 Lab 6
*       Description: 
*********************************************************************************************/

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

GD	*gp;
SB	*sp;
INODE	*ip;
DIRECTORY *dp;

char *path;
char *device;

int fd;
int iblock;

char *tokens[10];				// Path names can have a max of ten nodes
int numTokens;

// ********** 	MAIN	 ********** // 

int main (int argc, char *argv[])
{
	char buf[BLKSIZE], *cp, temp[256], *token;
	int index = 0, ino, bno;	

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
	get_block(fd, 1, buf);
	sp = (SB *)buf;

	if (sp->s_magic != 0xEF53)
	{
		printf("Sorry, %s is not an EXT2 File System.\n", device);
		exit(1);
	}

	// Read in group block, get InodesBeginBlock
	get_block(fd, 2, buf);
	gp = (GD *)buf;
	cp = buf;

	iblock = gp->bg_inode_table;

	// Read InodesBeginBlock to get root dir
	
	get_block(fd, iblock, buf);
	ip = (INODE *)buf + 1;		// root dir
	
	// Tokenize pathname
	
	tokenizePath(path);	

	// Search for token, move to dir, 

	get_block(fd, ip->i_block[0], buf);
	cp = (char *)buf;
	dp = (DIRECTORY *)buf;

	for (index = 0; index < numTokens; index++)
	{
printf("OUTER\n");
printf("First child: %s\n", dp->name);		

		strncpy(temp, dp->name, dp->name_len);
		temp[dp->name_len] = 0;

printf("temp: %s\n", temp);
printf("name_len: %d\n", dp->name_len);

		ino = search(ip, temp, index, cp);

		if (!ino) exit(1);

		// At this point, a match must have been found for the current token, move to that inode
		
		// Use mailman's algorithm to find inode
printf("ino before mm: %d\n", ino);
printf("bno before mm: %d\n", bno);
printf("iblock before mm: %d\n", iblock);

		bno = (ino - 1) / 8 + iblock;
		ino = (ino - 1) % 8;

printf("ino after mm: %d\n", ino);
printf("bno after mm: %d\n", bno);

		//get_block(fd, ino, buf);
		get_block(fd, bno, buf);
		ip = (INODE *)buf + ino;		

printf("Inode mode: %d\n", ip->i_mode);

		get_block(fd, ip->i_block[0], buf);
		cp = (char *)buf;
		dp = (DIRECTORY *)cp;

		//dp = (DIRECTORY *)ip;		

		//get_block(fd, ip->i_block[0], buf);
		//cp = (char *)buf;
		//dp = (DIRECTORY *)buf;		

printf("Test: %s\n", dp->name);

		if (index != numTokens - 1 && !S_ISDIR(ip->i_mode))		// If we are expecting a directory and it isn't one, exit
		{
			printf("%s is not a directory. Aborting.\n", tokens[index]);
			exit(1);
		}		
	}

	// INODE Found! Now just print it's info

	printf("Node Found!\n");
	printf("i_mode : \t%d\n", ip->i_mode);
	printf("i_uid : \t%d\n", ip->i_uid);
	printf("i_size : \t%d\n", ip->i_size);
	printf("i_gid : \t%d\n", ip->i_gid);
	printf("i_links_count : \t%d\n", ip->i_links_count);

	return 0;
}

// **********	  UTILITY FUNCTIONS	********** // 

int search(INODE *ip, char *name, int index, char *cp)
{
	int i, inode;
	//while (strcmp(name, tokens[index]) != 0)                // Loop until we find matching token
        for (i = 0; i < 12; i++)
	{
printf("INNER\n");
printf("Looking for: %s\n", tokens[index]);
printf("Current dp->name: %s\n", dp->name);
                if (strcmp(name, tokens[index]) == 0) 
		{
			inode = dp->inode;
			printf("Found %s.\n", name);
			return inode;
		}
			
		//if (!dp->name[0])				// If token wasn't found and we've run out of entries, invalid path 
                //{
               // 	printf("Invalid path.\n");
                //        return 0;
               	//}

                cp += dp->rec_len;                              // Otherwise, move on to the next one!
                dp = (DIRECTORY *)cp;

                strncpy(name, dp->name, dp->name_len);
                name[dp->name_len] = 0;
printf("temp: %s\n", name);
printf("name_len: %d\n", dp->name_len);
  	}

	return inode;
}

int get_block(int fd, int blk, char buf[])
{
	lseek(fd, (long)blk*BLKSIZE, 0);
	read(fd, buf, BLKSIZE);

	return 1;
}

int tokenizePath(char *path)
{
	char *token;
	int index = 1;
	
	numTokens = 0;

	if (path[0] == '/')				// If path starts with a '/', remove it to simplify tokens
		token++;

	token = strtok(path, "/");
	tokens[0] = (char *)malloc(strlen(token));		// Allocate and copy first token to global tokens array
	strcpy(tokens[0], token);

	numTokens++;

	while (token = strtok(NULL, "/"))		// Allocate and copy the rest of the tokens
	{
		tokens[index] = (char *)malloc(strlen(token));
		strcpy(tokens[index], token);
		numTokens++;
		index++;
	}

	return 1;
}



