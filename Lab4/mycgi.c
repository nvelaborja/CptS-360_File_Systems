#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#define MAX 10000
typedef struct {
    char *name;
    char *value;
} ENTRY;

ENTRY entry[MAX];

// Function Prototypes
void mymkdir(void);
void myrmdir(void);
void myrm(void);
void mycat(void);
void mycp(void);
void myls(void);

main(int argc, char *argv[]) 
{
  int i, m, r;
  char cwd[128];

  m = getinputs();    // get user inputs name=value into entry[ ]
  getcwd(cwd, 128);   // get CWD pathname

  printf("Content-type: text/html\n\n");
  printf("<p>pid=%d uid=%d cwd=%s\n", getpid(), getuid(), cwd);

  printf("You submitted the following name/value pairs:<p>");
 
  for(i=0; i <= m; i++)
     printf("%s = %s<p>", entry[i].name, entry[i].value);
  printf("<p>");

  	// Main block for command processing, entry[0].value is the user-entered portion in the first form box, should be the command name
	if (entry[0].value == "mkdir")
		mymkdir();	
	else if (entry[0].value == "rmdir")
		myrmdir();
	else if (entry[0].value == "rm")
		myrm();
	else if (entry[0].value == "cat")
		mycat();
	else if (entry[0].value == "cp")
		mycp();
	else if (entry[0].value == "ls")
		myls();
	else printf("<p>There seems to be an error with your entered command. Please make sure to only enter accepted commands\n.<p>");

 
  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#555555\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");
  
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs560.eecs.wsu.edu/~velaborja/cgi-bin/mycgi\">");

  //------ NOTE : CHANGE ACTION to YOUR login name ----------------------------
  //printf("<FORM METHOD=\"POST\" ACTION=\"http://cs560.eecs.wsu.edu/~YOURNAME/cgi-bin/mycgi\">");
  
  printf("Enter command : <INPUT NAME=\"command\"> <P>");
  printf("Enter filename1: <INPUT NAME=\"filename1\"> <P>");
  printf("Enter filename2: <INPUT NAME=\"filename2\"> <P>");
  printf("Clear commands: <INPUT TYPE=\"reset\" VALUE=\"Click to Reset\"><P>");
  printf("Submit command: <INPUT TYPE=\"submit\" VALUE=\"Click to Submit\"><P>");
  printf("</form>");
  printf("------------------------------------------------<p>");

  printf("</body>");
  printf("</html>");
}

void mymkdir(void)
{

}

void myrmdir(void)
{

}

void myrm(void)
{

}

void mycat(void)
{

}

void mycp(void)
{

}

void myls(void)
{

}

