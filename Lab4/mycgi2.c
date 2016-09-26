#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#define MAX 10000
typedef struct {
    char *name;
    char *value;
} ENTRY;

ENTRY entry[MAX];
struct stat mystat, *sp;

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
char ftime[64];

char *arguments[];		// Argument values only for use with execve()

// Function Prototypes
void mymkdir(void);
void myrmdir(void);
void myrm(void);
void mycat(void);
void mycp(void);
void myls(void);
void processArgs(void);
int ls_file(char *fname);
int ls_dir(char *dname);


main(int argc, char *argv[]) 
{
  int i, m, r;
  char cwd[128];

  m = getinputs();    // get user inputs name=value into entry[ ]
  getcwd(cwd, 128);   // get CWD pathname

  printf("Content-type: text/html\n\n");
  printf("<p>pid=%d uid=%d cwd=%s\n", getpid(), getuid(), cwd);

  printf("<p>You submitted the following name/value pairs:<p>");
 
  for(i=0; i <= m; i++)
     printf("%s = %s<p>", entry[i].name, entry[i].value);
  printf("<p>");

  	// Main block for command processing, entry[0].value is the user-entered portion in the first form box, should be the command name
  	
  	if (strcmp(entry[0].value, "mkdir") == 0)
		mymkdir();	
	else if (strcmp(entry[0].value, "rmdir") == 0)
		myrmdir();
	else if (strcmp(entry[0].value, "rm") == 0)
		myrm();
	else if (strcmp(entry[0].value, "cat") == 0)
		mycat();
	else if (strcmp(entry[0].value, "cp") == 0)
		mycp();
	else if (strcmp(entry[0].value, "ls") == 0)
		myls();
	else printf("<p>There seems to be an error with your entered command. Please make sure to only enter accepted commands\n.<p>");

 
  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#555555\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");
  
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~velaborja/cgi-bin/mycgi\">");
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
	char *path;
	char buff[128];
	int status;

	if (entry[1].value[0] != '/')		// Relative mode. entry[1] is the path for mkdir
	{
		path = getcwd(buff, 128);	// Get cwd
		strcat(path, "/");		// Add path seperator
		strcat(path, entry[1].value);	// Add rest of path
	}
	else strcpy(path, entry[1].value);	// Asolute mode, just take path as given second argument

	printf("Final Path for mkdir: %s<p>", path);

	status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);	// call system mkdir function
}

void myrmdir(void)
{
	char *path;
	char buff[128];
	int status;

	if (entry[1].value[0] != '/')		// Relative mode.
	{
		path = getcwd(buff, 128);	// get cwd
		strcat(path, "/");		// Add path seperator
		strcat(path, entry[1].value);	// Add rest of path
	}	
	else strcpy(path, entry[1].value);	// Absolute mode, just take path as the given second argument

	printf("Final Path for rmdir: %s<p>", path);

	status = rmdir(path);			// call system rmdir function
}

void myrm(void)
{
	char *path;
	char buff[128];
	int status;

	if (entry[1].value[0] != '/')		// Relative mode
	{
		path = getcwd(buff, 128);	// get cwd
		strcat(path, "/");		// Add path seperator
		strcat(path, entry[1].value);	// Add rest of path
	}
	else strcpy(path, entry[1].value);	// Absolute mode, just take path as the given second argument
	
	printf("Final path for rm: %s<p>", path);

	status = remove(path);
}

void mycat(void)
{
	char *path;
	char buff[128], fileLine[1024];		// Assumes no line in file will be longer than 1024 chars
	int status;
	FILE *file;

	if (entry[1].value[0] != '/')		// Relative mode
	{
		path = getcwd(buff, 128);	// get cwd
		strcat(path, "/");		// Add path seperator
		strcat(path, entry[1].value);	// Add rest of path
	}
	else strcpy(path, entry[1].value);	// Absolute mode, just take path as the given second argument

	printf("Final path for cat: %s<p>", path);

	file = fopen(path, "r");		// Open file with read permissions

	while (!feof(file))
	{
		fgets(fileLine, sizeof(fileLine), file);	// Get line from file
		printf("%s<p>", fileLine);			// Print line
	}

	fclose(file);				// Close file after reading

	printf("<p>");				// Print new line after file output
}

void mycp(void)
{
	char *sourcePath, *destPath;
	char sourceBuff[128], destBuff[128];
	int status, pid;

	// First sort out source file stuff
	if (entry[1].value[0] != '/')			// Relative mode	
	{
		sourcePath = getcwd(sourceBuff, 128);	// get cwd
		strcat(sourcePath, "/");		// Add path seperator
		strcat(sourcePath, entry[1].value);	// Add rest of path
	}
	else strcpy(sourcePath, entry[1].value);	// Absolute mode, just take path as the given second argument

	printf("Final Source Path for cp: %s<p>", sourcePath);

	// Next get destination file path 
	if (entry[2].value[0] != '/')			// Relative mode
	{
		destPath = getcwd(destBuff, 128);	// get cwd
		strcat(destPath, "/");			// Add path seperator
		strcat(destPath, entry[2].value);	// Add rest of path
	}
	else strcpy(destPath, entry[2].value);		// Absolute mode, just take path as the given third argument


	printf("Final Dest Path for cp: %s<p>", destPath);

	// Finally, fork process to do cp
	pid = fork();

	if (pid)					// Parent, just wait for child
	{
		wait(&pid);
	}	
	else						// Child process, call cp
	{
		execl("/bin/cp", "/bin/cp", sourcePath, destPath, (char *)0);	// Call cp
	}
}

void myls(void)
{
	struct stat mystat, *sp;
	int r;
	char *s;
	char name[1024], cwd[1024];

	s = entry[1].value;
	if (strlen(s) < 2) s = "./";

	sp = &mystat;
	if (r = stat(s, sp) < 0)
	{
		printf("<p>File \"%s\" does not exist.<p>\n", s);
		exit(1);
	}

	strcpy(name, s);
	if (s[0] != '/')		// Relative mode, must get cwd path
	{
		getcwd(cwd, 1024);
		strcpy(name, cwd);
		strcat(name, "/");
		strcat(name, s);
	}

	if (S_ISDIR(sp->st_mode)) ls_dir(name);
	else ls_file(name);
}

int ls_file(char *fname)
{
	struct stat fstat, *sp;
	int r; int i;

	sp = &fstat;

	if ( (r = stat(fname, &fstat)) < 0)
	{
		printf("<p>Can't stat %s<p>\n", fname);
		exit(1);
	}

	if ((sp->st_mode & 0x8000) == 0x8000) printf("%c", '-');
	if ((sp->st_mode & 0x4000) == 0x4000) printf("%c", 'd');
	if ((sp->st_mode & 0xA000) == 0xA000) printf("%c", 'l');

	for (i = 8; i > -1; i--)
	{
		if (sp->st_mode & (1<<i)) printf("%c", t1[i]);
		else printf("%c", t2[i]);
	}

	printf("\t\t%4d ", sp->st_nlink);
	printf("%4d ", sp->st_gid);
	printf("%4d ", sp->st_uid);
	printf("%8d ", sp->st_size);

	strcpy(ftime, ctime(&sp->st_ctime));
	ftime[strlen(ftime) - 1] = 0;
	printf("%s  ", ftime);
	printf("%s\n<p>", basename(fname)); 
}

int ls_dir(char *dname)
{
	char name[1024];
	DIR *dp;
	struct dirent *ep;

	// Open DIR to read names
	dp = opendir(dname);

	while (ep = readdir(dp))
	{
		if (!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")) continue;
		strcpy(name, dname);
		strcat(name, "/");
		strcat(name, ep->d_name);
		ls_file(name);
	}
}

void processArgs(void)
{
	int i = 0;

	// Copy entry values into arguments[]
	while (entry[i].value != "")
	{
		strcpy(arguments[i], entry[i].value);
		i++;
	}
}

