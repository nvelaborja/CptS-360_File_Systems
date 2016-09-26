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

#define  MAX 256

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
char ftime[64];

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables

char command[MAX];                                                                              // Container for input command
char arguments[10][256];                                                                                // Container for subsequent arguments in input line after command, maximum 10 for future use

// Server initialization code:

int server_init(char *name)
{
   	printf("==================== server init ======================\n");   
   	// get DOT name and IP address of this host

   	printf("1 : get and show server host info\n");
   	hp = gethostbyname(name);
   	if (hp == 0)
	{
      		printf("unknown host\n");
      		exit(1);
   	}
   	printf("    hostname=%s  IP=%s\n",
	hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
   	//  create a TCP socket by socket() syscall
   	printf("2 : create a socket\n");
   	mysock = socket(AF_INET, SOCK_STREAM, 0);
   	if (mysock < 0)
	{
      		printf("socket call failed\n");
      		exit(2);
   	}

   	printf("3 : fill server_addr with host IP and PORT# info\n");
   	// initialize the server_addr structure
   	server_addr.sin_family = AF_INET;                  // for TCP/IP
   	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
   	server_addr.sin_port = 0;   // let kernel assign port

   	printf("4 : bind socket to host info\n");
   	// bind syscall: bind the socket to server_addr info
   	r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
   	if (r < 0)
	{
       		printf("bind failed\n");
       		exit(3);
   	}

   	printf("5 : find out Kernel assigned PORT# and show it\n");
   	// find out socket port number (assigned by kernel)
   	length = sizeof(name_addr);
   	r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
   	if (r < 0)
	{
      		printf("get socketname error\n");
      		exit(4);
   	}

   	// show port number
   	serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
   	printf("    Port=%d\n", serverPort);

   	// listen at port with a max. queue of 5 (waiting clients) 
   	printf("5 : server is listening ....\n");
   	listen(mysock, 5);
   	printf("===================== init done =======================\n");
}


main(int argc, char *argv[])
{
   	char *hostname;
   	char line[MAX];

   	if (argc < 2)
      		hostname = "localhost";
   	else
      		hostname = argv[1];
 
   	server_init(hostname); 

   	// Try to accept a client request
   	while(1)
	{
     		printf("server: accepting new connection ....\n"); 

     		// Try to accept a client connection as descriptor newsock
     		length = sizeof(client_addr);
     		client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
     		if (client_sock < 0)
		{
        		printf("server: accept error\n");
        		exit(1);
     		}
     		printf("server: accepted a client connection from\n");
     		printf("-----------------------------------------------\n");
     		printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr), ntohs(client_addr.sin_port));
     		printf("-----------------------------------------------\n");

     		// Processing loop: newsock <----> client
     		while(1)
		{
       			n = read(client_sock, line, MAX);
       			if (n==0)
			{
        	   		printf("server: client died, server loops\n");
        	   		close(client_sock);
        	   		break;
      			}
      	
      			// show the line string
      			printf("server: read  n=%d bytes; line=[%s]\n", n, line);

			// Do stuff
			clearArguments();		// Clear arguments so old args don't interfere with new commands
			processInput(line);		// Populate command and arguments containers for command function use
			
			strcpy(line, "server: Line received from client. Command: ");
			//line = "Line received from client. Command: ";	// Begin client output format
			strcat(line, command);				// Add command to it
			if (strlen(arguments[0]) > 0)			// If an argument was received from client, add that as well
			{
				strcat(line, " | Argument: ");
				strcat(line, arguments[0]);
			}
			
			// Perform command specified by client
			if (strcmp(command, "mkdir") == 0)
			{
                                makedir();
			}
                        else if (strcmp(command, "rmdir") == 0)
                        {
			        removedir();
                        }
			else if (strcmp(command, "rm") == 0)
                        {
			        rm();
                        }
			else if (strcmp(command, "cat") == 0)
			{
                                cat();
			}
                        else if (strcmp(command, "cd") == 0)
			{
                                cd();
			}
                        else if (strcmp(command, "ls") == 0)
			{
                                ls();
			}
                        else if (strcmp(command, "pwd") == 0)
			{
                                pwd();		
			}
			else if (strcmp(command, "get") == 0)
			{
				myget();
			}
			else if (strcmp(command, "put") == 0)
			{
				myput();
			}
			else if (strcmp(command, "quit") == 0) 
			{
				printf("Quit command received from client. Exiting now.\n");
				exit(1);
			}
	
	      		// Let the client know we got the command request and completed it.
	      		n = write(client_sock, line, MAX);

			// Display amount of bytes written to the server log	
	      		printf("server: wrote n=%d bytes.\n", n);
	      		printf("server: ready for next request\n");
	    	}
 	}
}

int processInput(char *line)
{
        char *token;
        int i = 0;

        token = strtok(line, " ");              // Begin tokenizing input line
        strcpy(command, token);                 // Take first token as the command

        while (token = strtok(NULL, " "))       // Put the rest of the arguments in the argument list 
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

int myget(void)
{
	int fileSize, newsock;
	struct stat fstat, *sp;
	int r, i, n;
	char ftime[64], *filePath, buff[128], fileBuff[MAX];
	FILE *file;

	if (arguments[0][0] != '/')		// Relative path
	{
		filePath = getcwd(buff, 128);	// get cwd
		strcat(filePath, "/");		// Add path seperator	
		strcat(filePath, arguments[0]);	// add the rest of the relative path
	}
	else strcpy(filePath, arguments[0]);	// Absolute mode, just take the given path in the argument

	int fd = open(filePath, O_RDONLY);

	if (fd >= 0) 
	{        
        	lstat(filePath, &fstat);
                sprintf(fileBuff, "%d", fstat.st_size);
                write(newsock, fileBuff, sizeof(fileBuff));         // Write the size to the client
                bzero(fileBuff, sizeof(fileBuff)); fileBuff[sizeof(fileBuff) - 1] = '\0';

               int bytes = 0;
               
               	// reading a line from the file
               	while (n = read(fd, fileBuff, sizeof(fileBuff))) 
               	{          
               		if (n != 0) 
			{
                       	fileBuff[sizeof(fileBuff) - 1] = '\0';

                       	// Update bytes
                       	bytes += n;
                       	printf("wrote %d bytes\n", bytes);

                       	// Write the contents to the server
                       	write(newsock, fileBuff, sizeof(fileBuff));
                       	bzero(fileBuff, sizeof(fileBuff));
                       	fileBuff[sizeof(fileBuff) - 1] = '\0';
                   	}
               	} 
               
               	close(fd);
	}
        else 
        {
             	write(newsock, "BAD", sizeof("BAD"));
        }

}

int myput(void)
{
printf("Enter myput()\n");

}

int makedir(void)
{
        char *path;
        char buff[128];
        int status;

        if (arguments[0][0] != '/')             // Relative mode. arguments[0] is the path
        {
                path = getcwd(buff, 128);       // Get cwd
                strcat(path, "/");              // Add path seperator
                strcat(path, arguments[0]);     // Add the rest of the path (relative path)
        }
        else strcpy(path, arguments[0]);        // Absolute mode, just take the given path in the second arg

        status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);    // call system mkdir function

        return status;
}

int removedir(void)
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

int rm(void)
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

int cat(void)
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

int ls(void)
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

int pwd(void)
{
        char cwd[1024];
        int status;

        status = getcwd(cwd, 1024);             // Get cwd
        printf("%s\n", cwd);                    // Print it

        return status;
}

int cd(void)
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


