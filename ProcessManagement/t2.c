#include <stdio.h>
#include <stdlib.h>

main()
{
	int pid, status;
	pid = fork();
	
	if (pid)
	{
		printf("PARENT %d WAITS FOR CHILD %d TO DIE\n", getpid(), pid);
		pid = wait(&status);
		printf("DEAD CHILD = %d, HOW = %04x\n", pid, status);
	}
	else
	{
		printf("child %d dies by exit(VALUE)\n", getpid());
		exit(500);   
		
	}

}
