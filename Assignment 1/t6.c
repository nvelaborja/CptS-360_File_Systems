#include <stdio.h>

// ****** t6.c file ******* 
int g[10000] = {4}; 
main()
{
	static int a, b, c, d[10000]; 
	a = 1; b = 2;
	c = a + b;
	printf("c=%d\n", c); 
}
