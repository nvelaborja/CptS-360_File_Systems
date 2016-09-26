/************* t.c file ********************/
#include <stdio.h>
#include <stdlib.h>

int *FP;

main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  printf("enter main\n");
  
  printf("&argc=%x argv=%x env=%x\n", (unsigned int) &argc, (unsigned int) argv, (unsigned int) env);
  printf("&a=%8x &b=%8x &c=%8x\n",(unsigned int) &a,(unsigned int) &b,(unsigned int) &c);

  a=1; b=2; c=3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");
  printf("Addresses of d, e, & f:\n&d=%8x &e=%8x &f=%8x\n", (unsigned int) &d, (unsigned int) &e, (unsigned int) &f); 
  d=4; e=5; f=6;
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");
  printf("Addresses of g, h and i: \n&g=%8x &h=%8x &i=%8x\n", (const int) &g, (const int) &h, (const int) &i); 
  g=7; h=8; i=9;
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p;
  int index = 1;

  printf("enter C\n");
  printf("Addresses of u, v, w, i, and p: \n&u=%8x &v=%8x &w=%8x &i=%8x &p=%8x\n", (const int) &u, (const int) &v, (const int) &w, (const int) &i, (const int) p); 
  u=10; v=11; w=12; i=13; p=14;

  p = (int *)&p;
  printf("p = %8x\n",(unsigned int) p);

  //Problems 1 through 4
  // Part 1. Print the stack frame link list
  asm("movl %ebp, FP");		// set FP=CPU's %ebp register 
  printf("Stack Frame Link List:\n"); 
  printf("Frame Link List (0) (FP) = %8x\n", FP);
  
  p = FP; 			// set p to FP to preserve FP's location 
  while (p != NULL)
  {
	// Print the current spot of the FP
	printf("Frame Link List (%d): %8x\n", index, *p);
	// Move the FP to the next link
	p = *p;
	// Increment index
	index++;
  }
  p = FP - 8;			// reset p to its initial location
 
  // Part 2. Print the stack contents from p to the frame of main()
  
  printf("Stack Contents:\nStack Index\tStack Value\t\t Address\n");
  for (index = 0; index < 128; index++)
  {
	printf("Stack[%d]:\t%8x\t@\t%8x\n", index, (const int) *p, (const int) p);
	p++;
  }  


}







