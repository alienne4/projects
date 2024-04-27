#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void openDirectories(char *directorPath)
{
    DIR *director=opendir(directorPath);

    if(director==NULL)
    {
        printf("Acesta este un mesaj de eroare");
        return;
    }

    struct dirent *entry;


    while ((entry = readdir(director)) != NULL)
    {
        struct stat buf;
        char path[1024];
        char filename[1024]="";

        switch (entry->d_type)
        {

            case DT_DIR:
                if (!(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
                {
                     snprintf(path, sizeof(path), "%s/%s", directorPath, entry->d_name);
                     openDirectories(path);
                }
                break;

            case DT_REG:   
                sprintf(filename,"%s/%s",directorPath,entry->d_name);

                char snapshotFilename[1024]="";
                sprintf(snapshotFilename, "%s.snapshot", filename);
                

                if(stat(filename, &buf) < 0)
                {
                    printf("Error stat2 \n");
                    exit(-1);
                }

                int fptr;

                fptr = open(snapshotFilename, O_CREAT | O_WRONLY | O_RDONLY);

                char snapshotContent[1024] = "";

                sprintf(snapshotContent, "Entry: %s\nSize: %d\nInode number: %d\n", filename, buf.st_size, buf.st_ino);

                ssize_t result = write(fptr, &snapshotContent, strlen(snapshotContent) - 1);

                // printf("Timestamp: %s\n", buf.st_atimespec.tv_sec);
                            /// printf("Entry: %s\n", filename);
                            ///printf("Size: %d\n", buf.st_size);
                            // printf("Last Modified: %s\n",  buf.st_mtimespec.tv_sec);
                            // printf("Permissions: %s\n", filename);
                            ///printf("Inode number: %d\n", buf.st_ino);

                close(fptr);
               
                break;

        }
    }

    closedir(director);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage <exec> dir N\n");
        exit(-1);
    }

    openDirectories(argv[1]);

    return 0;
}