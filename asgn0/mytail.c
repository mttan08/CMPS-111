// Matthew Tan
// CMPS 111
// asgn0: mytail.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

off_t fileSize;
off_t curPos = 0;

#define BUF_SIZE 8192 * 20

int stringlength(const char *s) 
{
    int count = 0;
    while (s[count] != '\0') 
    {
        count++;
    }
    return count;
}

int readline(int fd, char *buffer, size_t len, size_t* readLen)
{
    memset(buffer, 0, len);
    *readLen = 0;

    char t;

    for (size_t i = 0; i < len; i++)
    {   
        curPos++;
        read(fd, &t, 1);

        if (curPos < fileSize) 
        {   
            if (t == '\n')
            {   
                buffer[i] = t;
                buffer[i + 1] = '\n';
                *readLen = i + 1;
                return *readLen;
            }   
            else
            {
                buffer[i] = t;
                *readLen = i;
            }   
        }
        else
        {
            buffer[i] = t;
            buffer[i + 1] = '\n';
            *readLen = i + 1;
            return -1;
        }   
    }
    return -1; 
}

int main(int argc, char* argv[])
{
    for (int fileIdx = 1; fileIdx < argc; fileIdx++)
    {
        int fd = open(argv[fileIdx], O_RDONLY);        
        if (fd < 0)
        {
            perror("File could not be opened!");
            return 2;
        }

        struct stat st;
        stat(argv[fileIdx], &st);
        fileSize = st.st_size;
        curPos = 0;
        int byteRead = 0;
        int lineNo = 0;    
        int done = 0;
        char* buffer;
        char* lineList[10];
        size_t readLenList[10];

        for (int i = 0; i < 10; i++)
        {
            lineList[i] = NULL;
            readLenList[i] = 0;
        }

        size_t totalBytes = 0;
        while (!done)
        {
            buffer = (char*)malloc(BUF_SIZE*sizeof(char*));
            size_t readLen = 0;
            byteRead = readline(fd, buffer, BUF_SIZE, &readLen);
            totalBytes += readLen;

            if (lineNo < 10)
            {
                lineList[lineNo] = buffer;
                readLenList[lineNo] = readLen;
            }
            else
            {
                int j = 0;
                for (j = 0; j < 9; j++)
                {
                    lineList[j] = lineList[j + 1];
                    readLenList[j] = readLenList[j + 1];
                }
                lineList[j] = buffer;
                readLenList[j] = readLen;
            }
            lineNo++;

            if (byteRead < 0)
            {
                done = 1;
                break;
            }
        }

        if (argc == 2)
        {
            size_t totalByteRead = 0;
            for (int i = 0; i < 10; i++)
            {
                if (lineList[i] != NULL)
                {
                    write(1, lineList[i], readLenList[i]);
                    totalByteRead += readLenList[i];
                }
            }
        }
        else
        {
            size_t totalByteRead = 0;
            char* farrow = "==> ";
            char* barrow = " <==";
            write(1, farrow, stringlength(farrow));
            write(1, argv[fileIdx], stringlength(argv[fileIdx]));
            write(1, barrow, stringlength(barrow));
            write(1, "\n", 1);
            for (int i = 0; i < 10; i++)
            {
                if (lineList[i] != NULL)
                {
                    write(1, lineList[i], readLenList[i]);
                    totalByteRead += readLenList[i];
                }
            }
            write(1, "\n", 1);
        }
        close(fd);
    }
}

