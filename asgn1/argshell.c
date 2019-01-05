// Matthew Tan
// mxtan
// CMPS 111
// asgn1

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

int idx = 0;

extern char **get_args();

int main()
{
    int i;
    char **args;
    while (1)
    {
	printf ("Command ('exit' to quit): ");
        args = get_args();
        idx = 0;

	for (i = 0; args[i] != NULL; i++) 
        {
	    printf("Argument %d: %s\n", i, args[i]);            
	}

	if (args[0] == NULL) 
        {
	    printf("No arguments on line!\n");
	} 
        else if (!strcmp (args[0], "exit")) 
        {
	    printf("Exiting...\n");
	    break;
	}

        int done = 0;

        while (!done)
        {
            int pipeFd_1[2];
            int pipeIn = 0;
            int cmdDone = 0;
            int nextCmdIdx = 0;
            int pipeOut = 0;
            int pipeFd_2[2];
            int currInd = 0;
            int semicolon = 0;
            int i;
            for (i = 0; args[i] != NULL; i++) 
            {
            }
            while (!cmdDone) 
            {
                int status;
                int redirectError = 0;
                char cwd[256];
                pipeOut = 0;
                int readFd = -1;
                int writeFd = -1;
                //printf("nextCmdIdx is: %d\n", nextCmdIdx);
                //printf("before for loop: i is: %d\n", i);
                int argc;
                for (argc = nextCmdIdx; argc < i; argc++)
                {                  
                    idx++;                         
                    //printf("index is: %d\n", idx);
                    //printf("args[argc] is: %s, argc is: %d\n", args[argc], argc);
                    if (strcmp(args[argc], "<") == 0)
                    {
                        args[argc] = NULL;
                        readFd = open(args[argc + 1], O_RDONLY);
                        if (readFd < 0)
                        {
                            perror("readFd can't be opened");
                            exit(1);
                        }
                        idx++;
                        break;
                    }
                    else if (strcmp(args[argc], ">") == 0)
                    {
                        if (strcmp(args[argc + 1], "&") == 0)
                        {
                            redirectError = 1;
                            argc = argc + 1;
                        }
                        args[argc] = NULL;
                        int tmp = argc;
                        writeFd = open(args[argc + 1], O_RDWR | O_CREAT, 0777);
                        //printf("writeFd is: %d\n", writeFd); 
                        if (writeFd < 0)
                        {
                            perror("writeFd can't be opened");
                            //printf("args[argc + 1] is: %s\n", args[argc + 1]);
                            exit(1);
                        }
                        idx++;
                    }
                    else if (strcmp(args[argc], ">&") == 0)
                    {
                        args[argc] = NULL;
                        int tmp = argc;
                        writeFd = open(args[argc + 1], O_RDWR | O_CREAT, 0777);
                        //printf("writeFd is: %d\n", writeFd); 
                        if (writeFd < 0)
                        {
                            perror("writeFd can't be opened");
                            //printf("args[argc + 1] is: %s\n", args[argc + 1]);
                            exit(1);
                        }
                        //printf("inside: args[tmp] is: %s\n", args[tmp]);
                        redirectError = 1;
                        //printf("in case >&, redirectError is: %d\n", redirectError);
                        idx++;
                        break;
                    }
                    else if (strcmp(args[argc], ">>") == 0)
                    {
                        args[argc] = NULL;
                        writeFd = open(args[argc + 1], O_RDWR | O_APPEND | O_CREAT, 
                                0777);
                        if (writeFd < 0)
                        {
                            perror("writeFd can't be opened");
                            exit(1);
                        }
                        idx++;
                        break;
                    }
                    else if (strcmp(args[argc], ">>&") == 0)
                    {
                        args[argc] = NULL;
                        writeFd = open(args[argc + 1], O_RDWR | O_APPEND | O_CREAT,
                            0777);
                        if (writeFd < 0)
                        {
                            perror("writeFd can't be opened");
                            exit(1);
                        }
                        redirectError = 1;
                        idx++;
                        break;
                    }
                    else if (strcmp(args[argc], "|") == 0)
                    {
                        args[argc] = NULL;                
                        pipeOut = 1;
                        nextCmdIdx = argc + 1;
                        //printf("pipe: nextCmdIdx: %d\n", nextCmdIdx);
                        if (pipe(pipeFd_2) == -1)
                        {
                            perror("pipe can't be opened");
                            exit(1);
                        }
                        break;
                    }
                    else if (strcmp(args[argc], "|&") == 0)
                    {
                        args[argc] = NULL;                
                        pipeOut = 1;
                        nextCmdIdx = argc + 1;
                        if (pipe(pipeFd_2) == -1)
                        {
                            perror("pipe can't be opened");
                            exit(1);
                        }
                        redirectError = 1;                
                        break;
                    }
                    else if (strcmp(args[argc], "cd") == 0)
                    {
                        getcwd(cwd, sizeof(cwd));
                        //printf("cwd is: %s\n", cwd);
                        //printf("args[argc + 1] is: %s\n", args[argc + 1]);
                        if (args[argc + 1] == NULL)
                        {
                            //printf("got in case args[argc + 1]!!!\n");
                            char* dir = getenv("HOME");
                            chdir(dir);
                        }
                        else
                        {
                            chdir(args[argc + 1]);
                        }
                    }
                    else if (strcmp(args[argc], ";") == 0)
                    {
                        semicolon = 1;
                        //printf("in case: ;, argc is: %d\n", argc);
                        args[argc] = NULL;
                        nextCmdIdx = argc + 1;
                        break;
                    }
                }

                pid_t pid = fork();

                if (pid == 0)
                {
                    //printf("In child, pid is: %d\n", pid);
                    if (readFd != -1)
                    {
                        close(0);
                        dup(readFd);
                    }
                    if (writeFd != -1)
                    {
                        //printf("redirectError is: %d\n", redirectError);
                        close(1);
                        dup(writeFd);
                        if (redirectError)
                        {
                            //printf("got in case redirectError!!!\n");
                            close(2);
                            dup(writeFd);
                        }
                    }
                    else if (pipeOut || pipeIn)
                    {
                        if (pipeIn)
                        {
                            close(0);
                            //printf("pipeIn: pipeFd_1[0] is: %d, pipeFd_1[1] is: %d\n", 
                            //       pipeFd_1[0], pipeFd_1[1]);
                            dup(pipeFd_1[0]);
                            close(pipeFd_1[0]);
                            close(pipeFd_1[1]);
                        }
                        if (pipeOut)
                        {
                            close(1);
                        //printf("pipeOut: pipeFd_2[0] is: %d, pipeFd_2[1] is: %d\n", 
                            //       pipeFd_2[0], pipeFd_2[1]);
                            dup(pipeFd_2[1]);
                            close(pipeFd_2[0]);
                            close(pipeFd_2[1]);
                            pipeOut = 0;
                        }
                    }
                    //printf("in child, execvp: args[0]: %s\n", args[0]);
                    if (execvp(args[currInd], &args[currInd]) == -1)
                    {
                        perror("In child: execvp() is not working!\n");
                        exit(1);
                    }
                    //printf("end of child!\n");
                }
                else
                {
                    //printf("In parent, pid is: %d\n", pid);
                    while (wait(&status) != pid); 
                    if (semicolon)
                    {
                        currInd = nextCmdIdx;
                        //printf("In parent: currInd is: %d\n", currInd);
                    }
                    if (pipeOut)
                    {
                        close(pipeFd_2[1]);
                        //printf("in parent, calling executeArgs: %s\n", 
                        //    args[nextCmdIdx]);
                        pipeFd_1[0] = pipeFd_2[0];
                        pipeFd_1[1] = pipeFd_2[1];
                        pipeIn = pipeOut;
                        currInd = nextCmdIdx;
                    }
                    //printf("in parent, end of parent!\n");
                }                
                //if (args[idx] == NULL || args[idx + 1] == NULL)
                        //if (args[currInd - 1] == NULL)
                //printf("In parent: args[idx]: %s, idx is: %d\n", 
                //       args[idx], idx);
                if (args[idx] == NULL)
                {
                    cmdDone = 1;
                }
            }
            if (args[idx] == NULL)
            {
                done = 1;
            }
        }
    }
    return 0;
}
