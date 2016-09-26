#include <stdio.h>
#include <stdlib.h>

typedef unsigned int u32;

int main (void)
{
        char string[20] = "Nathan is cool!";

        // testing for part 1
        prints("Part One:\n");
        prints(string);
        prints("\n");

        // testing for code given in part 2
        prints("Part Two:\n");
        u32 num1 = 11392441;
        prints("printu():");
        printu(num1);
        prints("\n");

        // testing for part 3
        prints("Part Three:\n");
        prints("printd():");
        printd(num1);
        printd(-num1);
        prints("\n");
        prints("printo():");
        printo(num1);
        printo(num1 - num1);
        prints("\n");
        prints("printx():");
        printx(num1);
        printx(num1 - num1);
        prints("\n");

        // testing for part 4
        prints("Part Four:\n");
        myprintf("Does this string with no variables work?");
        prints("\n");
        myprintf("What\nabout\nnew\nlines\n?\n");
        myprintf("c:%c  s:%s  u:%u  d:%d  o:%o  x:%x", '1', "two", 3, -4, 5, 6);
        prints("\n");
        return 0;
}

