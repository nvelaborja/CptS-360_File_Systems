#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#define MAX 256
#define COMMANDS 8

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
char ftime[64];

int server_sock, r;
int SERVER_IP, SERVER_PORT; 

char *localCommands[8] = {"lcat", "lpwd", "lls", "lcd", "lmkdir", "lrmdir", "lrm", "quit"};	// List of supported local commands
char *serverCommands[8] = {"pwd", "ls", "cd", "mkdir", "rmdir", "rm", "get", "put"};		// List of supported server commands
char command[MAX];										// Container for input command
char arguments[10][256];										// Container for subsequent arguments in input line after command, maximum 10 for future use

// Local / Server Command Check Functions

int isLocalCommand(char *string)
{
	int i = 0;

	while (i < COMMANDS)					// Loop through all the local commands
	{	
		if (strcmp(string, localCommands[i]) == 0)	// If the input string matches any, return true
			return 1;		
		i++;						// Move to next command
	}

	return 0;						// Return false if string did not match any commands
}

int isServerCommand(char *string)
{
	int i = 0;

	while (i < COMMANDS)					// Loop through all the server commands
	{
		if (strcmp(string, serverCommands[i]) == 0)	// If the input string matches any, return true
			return 1;
		i++;						// Move to next command
	}
	
	return 0;						// Return false if string did not match any commands
}

// clinet initialization code

int client_init(char *argv[])
{
	printf("======= clinet init ==========\n");

  	printf("1 : get server info\n");
  	hp = gethostbyname(argv[1]);
  	if (hp==0)
	{
		printf("unknown host %s\n", argv[1]);
  		exit(1);
  	}

  	SERVER_IP   = *(long *)hp->h_addr;
  	SERVER_PORT = atoi(argv[2]);
	
	printf("2 : create a TCP socket\n");
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock<0)
	{
		printf("socket call failed\n");
	     	exit(2);
	}
	
	printf("3 : fill server_addr with server's IP and PORT#\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = SERVER_IP;
	server_addr.sin_port = htons(SERVER_PORT);
	
	// Connect to server
	printf("4 : connecting to server ....\n");
	r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
	if (r < 0)
	{
		printf("connect failed\n");
	   	exit(1);
	}
	
	printf("5 : connected OK to \007\n"); 
	printf("---------------------------------------------------------\n");
	printf("hostname=%s  IP=%s  PORT=%d\n", hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
	printf("---------------------------------------------------------\n");
	
	printf("========= init done ==========\n");
}
	
int main(int argc, char *argv[ ])
{
	int n;
	char line[MAX], ans[MAX], lineCp[MAX];
	
	if (argc < 3)
	{
		printf("Usage : client ServerName SeverPort\n");
	     	exit(1);
	}
	
	client_init(argv);
	// sock <---> server
	printf("********  processing loop  *********\n");
	while (1)
	{
		// Get input line
	    	printf("input a line : ");
	    	bzero(line, MAX);                // zero out line[ ]
	    	fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
	    	line[strlen(line)-1] = 0;        // kill \n at end
	    	if (line[0]==0)                  // exit if NULL line
	       		exit(0);
		
		// Process input line
		strcpy(lineCp, line);		// Use copy of line so line remains intact
		clearArguments();
		processInput(lineCp);
				
		// Local Commands
		if (isLocalCommand(command))
		{
			if (strcmp(command, "lmkdir") == 0)
 	        	       	lmkdir();
 		       	else if (strcmp(command, "lrmdir") == 0)
 		               	lrmdir();
 		       	else if (strcmp(command, "lrm") == 0)
 		               	lrm();
 		       	else if (strcmp(command, "lcat") == 0)
 		               	lcat();
 		       	else if (strcmp(command, "lcd") == 0)
 		               	lcd();
 		       	else if (strcmp(command, "lls") == 0)
 		               	lls();
			else if (strcmp(command, "lpwd") == 0)
				lpwd();	
			else if (strcmp(command, "quit") == 0)
			{
				n = write(server_sock, line, MAX);
				exit(1);
			}

			continue;
		}
	
		// Server Commands
		if (isServerCommand(command))
		{
			n = write(server_sock, line, MAX);		// Send input line to server

                        if (strcmp(command, "get") == 0)
                        {
                                int fileSize, n, count = 0;
				FILE *newFile;
				char *buff[MAX];		// Buffer for file writing 

                                n = read(server_sock, buff, sizeof(fileSize));	// Read the file size from server
				printf("File Size Received From Server: %d\n", fileSize);
				if (strcmp(buff, "BAD") == 0)
				{
					printf("Server could not stat given file path.\n");
					continue;
				}
				
				fileSize = atoi(buff);

				int fd = open(arguments[0], O_WRONLY | O_CREAT, 0644);
				int bytes = 0;
				
				while (bytes < fileSize)
				{
					n = read(server_sock, buff, MAX);		// Read into buffer from server
					bytes += n;					// Updated byte counter
					write(fd, buff, MAX);
					bzero(buff, MAX);
					buff[sizeof(buff) - 1] = '\0';
				}				

				fclose(fd);					// Close file when we're all done

                                continue;
                        }

                        if (strcmp(command, "put") == 0)
                        {

                                continue;
                        }

			// All other server commands do not require any other work from the client
		
			n = read(server_sock, ans, MAX);		// Get answer back from server
			
			printf("%s\n", ans);				// Display server answer
			continue;
		}
		
		printf("\nCommand not recognized. Please enter an accepted local or server command.\n");
		continue;

	}
}

int processInput(char *line)
{
	char *token;
	int i = 0;
	
	token = strtok(line, " ");		// Begin tokenizing input line
	strcpy(command, token);			// Take first token as the command

	while (token = strtok(NULL, " "))	// Put the rest of the arguments in the argument list 
	{
		if (token && i < 10)
		{
			strcpy(arguments[i], token);
			i++;
			continue;	
		}
	}

	return 1;
}

// Local Command Functions

int lmkdir(void)
{
	char *path;
	char buff[128];
	int status;
	
	if (arguments[0][0] != '/')		// Relative mode. arguments[1] is the path
	{
		path = getcwd(buff, 128);	// Get cwd
		strcat(path, "/");		// Add path seperator
		strcat(path, arguments[0]);	// Add the rest of the path (relative path)
	}
	else strcpy(path, arguments[0]);	// Absolute mode, just take the given path in the second arg

	status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);	// call system mkdir function
	
	return status;
}

int lrmdir(void)
{
        char *path;
        char buff[128];
        int status;

	if (arguments[0][0] != '/')           // Relative mode.
        {
                path = getcwd(buff, 128);       // get cwd
                strcat(path, "/");              // Add path seperator
                strcat(path, arguments[0]);   // Add rest of path
        }
        else strcpy(path, arguments[0]);      // Absolute mode, just take path as the given second argument

        status = rmdir(path);                   // call system rmdir function
	
	return status;
}

int lrm(void)
{
        char *path;
        char buff[128];
        int status;

        if (arguments[0][0] != '/')           // Relative mode
        {
                path = getcwd(buff, 128);       // get cwd
                strcat(path, "/");              // Add path seperator
                strcat(path, arguments[0]);   // Add rest of path
        }
        else strcpy(path, arguments[0]);      // Absolute mode, just take path as the given second argument

        status = remove(path);
	
	return status;
}

int lcat(void)
{
        char *path;
        char buff[128], fileLine[1024];         // Assumes no line in file will be longer than 1024 chars
        int status;
        FILE *file;

        if (arguments[0][0] != '/')           // Relative mode
        {
                path = getcwd(buff, 128);       // get cwd
                strcat(path, "/");              // Add path seperator
                strcat(path, arguments[0]);   // Add rest of path
        }
        else strcpy(path, arguments[0]);      // Absolute mode, just take path as the given second argument

        file = fopen(path, "r");                // Open file with read permissions

        while (!feof(file))
        {
                fgets(fileLine, sizeof(fileLine), file);        // Get line from file
                printf("%s", fileLine);                      // Print line
        }

        fclose(file);                           // Close file after reading

        printf("\n");                          // Print new line after file output
}

int lls(void)
{
        struct stat mystat, *sp;
        int r;
        char *s;
        char name[1024], cwd[1024];

        s = arguments[1];
	
        if (strlen(s) < 2) s = "./";

        sp = &mystat;
        if (r = stat(s, sp) < 0)
        {
                printf("File \"%s\" does not exist.\n", s);
                exit(1);
        }

        strcpy(name, s);
        if (s[0] != '/')                // Relative mode, must get cwd path
        {
                getcwd(cwd, 1024);
                strcpy(name, cwd);
                strcat(name, "/");
                strcat(name, s);
        }

        if (S_ISDIR(sp->st_mode)) lls_dir(name);
        else lls_file(name);
}

int lls_file(char *fname)
{
        struct stat fstat, *sp;
        int r; int i;

        sp = &fstat;

        if ( (r = stat(fname, &fstat)) < 0)
        {
                printf("Can't stat %s\n", fname);
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

        printf("\t%4d ", sp->st_nlink);
        printf("%4d ", sp->st_gid);
        printf("%4d ", sp->st_uid);
        printf("%8d ", sp->st_size);

        strcpy(ftime, ctime(&sp->st_ctime));
        ftime[strlen(ftime) - 1] = 0;
        printf("%s  ", ftime);
        printf("%s\n", basename(fname));
}

int lls_dir(char *dname)
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
                lls_file(name);
        }
}

int lpwd(void)
{
	char cwd[1024];
	int status;

	status = getcwd(cwd, 1024);		// Get cwd
	printf("%s\n", cwd);			// Print it

	return status;
}

int lcd(void)
{
        char *path;
        char buff[128];
        int status;

        if (arguments[0][0] != '/')           // Relative mode
        {
                path = getcwd(buff, 128);       // get cwd
                strcat(path, "/");              // Add path seperator
                strcat(path, arguments[0]);   // Add rest of path
        }
        else strcpy(path, arguments[0]);      // Absolute mode, just take path as the given second argument

        status = chdir(path);

        return status;
	
}

int clearArguments(void)
{
	int i = 0;

	while (i < 10)
	{
		bzero(arguments[i], 256);
		i++;
	}
	return 1;
}
