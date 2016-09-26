#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int u32;

//int BASE = 10;	// Added as parameter to rpu so that it can be modified depending on use
char *table = "0123456789ABCDEF";

void prints(char *s)	// Prints a string using putchar()
{
	// Loop through the string till we find the null character to terminate the string
	while (*s != '\0')
	{
		putchar(*s);
		s++;
	}
}

int rpu(u32 x, int BASE) // Given function, returns formated number based on given base
{
	char c;
	
	if (x)
	{
		c = table[x % BASE];
		rpu(x / BASE, BASE);
		putchar(c);
	}
}

int printu(u32 x) 	// prints an unsigned integer
{
	if (x==0)
		putchar('0');
	else
		rpu(x, 10);
	putchar(' ');
}

int printd(int x)	// prints a signed integer
{
	// If the value is negative, print a negative sign then flip the number to be positive so rpu will work
	if (x < 0)
	{
		putchar('-');
		x = -x;
	}
	rpu(x, 10);
	putchar(' ');
}

int printo(u32 x)	// prints an unsigned integer in octal form
{
	if (x == 0)
		prints("00000000");
	else
	{
		putchar('0');		// Since we're only dealing with unsigned ints, sign bit will always be 0
		rpu(x, 8);
	}
	putchar(' ');
}

int printx(u32 x)
{
	if (x == 0)
		prints("0x0000000");
	else
	{
		putchar('0'); putchar('x');	// Hex format. Again, we will only be dealing with unsigned ints so sign bit will always be 0
		rpu(x, 16);
	}
	putchar(' ');
}

int myprintf(char *fmt, ...)
{
	char *cp = fmt;
	int *ip = &fmt + 1;
	
	while (*cp != '\0')
	{
		if (*cp == '%')		// If we find a percent, grab next char which must be c, s, u, d, o, or x
		{
			cp++;		// Move to next character
			switch (*cp)
			{
				case 'c':
					putchar(*ip);
					break;
				case 's':
					prints(*ip);
					break;
				case 'u':
					printu(*ip);
					break;
				case 'd':
					printd(*ip);
					break;
				case 'o':
					printo(*ip);
					break;
				case 'x':
					printx(*ip);
					break;
				default:
					prints("Error with \% usage");
			}
			ip++;		// Advance ip to the next item on the stack
		}
		else
		{
			putchar(*cp);
			if (*cp == '\n')	// If we find a new line, spit out an extra '\r'
				putchar('\r');
		}		
		
		cp++; 			// Move to the next item in the input string
	
	}	
}
