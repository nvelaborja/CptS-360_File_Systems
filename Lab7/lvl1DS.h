#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct minode
{
	char		dev[256];
	int		ino;
	int		refCount;
	int		dirty;
} MINODE;

typedef struct proc
{
	struct proc	*nextProcPtr;
	int		pid;
	int		uid;
	MINODE		*cwd;
	char		fd[10];
} PROC;


