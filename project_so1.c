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
        printf("Acesta este un mesaj de eroaore");
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

                char filePermissions[11];

                filePermissions[0] = '-';
                filePermissions[1] = (buf.st_mode & S_IRUSR) ? 'r' : '-';
                filePermissions[2] = (buf.st_mode & S_IWUSR) ? 'w' : '-';
                filePermissions[3] = (buf.st_mode & S_IXUSR) ? 'x' : '-';
                filePermissions[4] = (buf.st_mode & S_IRGRP) ? 'r' : '-';
                filePermissions[5] = (buf.st_mode & S_IWGRP) ? 'w' : '-';
                filePermissions[6] = (buf.st_mode & S_IXGRP) ? 'x' : '-';
                filePermissions[7] = (buf.st_mode & S_IROTH) ? 'r' : '-';
                filePermissions[8] = (buf.st_mode & S_IWOTH) ? 'w' : '-';
                filePermissions[9] = (buf.st_mode & S_IXOTH) ? 'x' : '-';
                filePermissions[10] = '\0';
   

                sprintf(snapshotContent, "Timestamp: %lld\nEntry: %s\nSize: %d\nLast Modified: %lld\nPermissions: %s\nInode number: %d\n", buf.st_atimespec.tv_sec, filename, buf.st_size, buf.st_mtimespec.tv_sec, filePermissions, buf.st_ino);

                write(fptr, &snapshotContent, strlen(snapshotContent) - 1);

                printf("Snapshot for %s created successfully\n", entry->d_name);

                close(fptr);

                pid_t pid = fork();

                if (pid < 0) {
                    perror("fork");
                    return;
                }

                if (pid == 0) {
                    execlp("/bin/sh", "sh", "/Users/adelinchis/Desktop/verify_for_malicious.sh", filename, NULL);

                    printf("execlp error\n");
                } 

               
                break;
        }
    }

    closedir(director);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage <exec> dir1 dir2 dir3 ... N\n");
        exit(-1);
    }

    pid_t pid[argc-1];
    int status;

    for(int i = 1; i < argc; i++){
        if( (pid[i-1] = fork()) < 0){
            printf("Cannot create child process\n");
            exit(1);
        }
        if(pid[i-1] == 0){
          
            openDirectories(argv[i]);
            exit(0);
        }
    }

    for(int i = 1; i < argc; i++){
        wait(&status);
        if(WIFEXITED(status) && pid[i-1] != 0)
            printf("Child process %d terminated with PID %d terminated with exit code %d.\n", i, pid[i-1], WEXITSTATUS(status));
    }

    return 0;
}