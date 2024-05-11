#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void openDirectories(char *directorPath, int pipeWrite){
    DIR *director=opendir(directorPath);

    if(director==NULL)
    {
        printf("Acesta este un mesaj de eroare\n");
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
                     openDirectories(path, pipeWrite);
                }
                break;

            case DT_REG:   
                sprintf(filename,"%s/%s",directorPath,entry->d_name);

                char snapshotFilename[1024]="";
                sprintf(snapshotFilename, "%s.snapshot", filename);

                if (access(snapshotFilename, F_OK) != -1) {
                    printf("Snapshot already exists for %s\n", filename);
                    continue;
                }

                int fptr = open(snapshotFilename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fptr < 0) {
                    perror("open");
                    return;
                }

                char snapshotContent[1024] = "";

                if(stat(filename, &buf) < 0)
                {
                    printf("Error stat2 \n");
                    exit(-1);
                }

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

                sprintf(snapshotContent, "Timestamp: %lld\nEntry: %s\nSize: %lld\nLast Modified: %lld\nPermissions: %s\nInode number: %lu\n", 
                        (long long)buf.st_atime, filename, (long long)buf.st_size, (long long)buf.st_mtime, 
                        filePermissions, (unsigned long)buf.st_ino);

                write(fptr, &snapshotContent, strlen(snapshotContent) - 1);

                printf("Snapshot for %s created successfully\n", entry->d_name);

                char prevSnapshotFilename[1024]="";
                sprintf(prevSnapshotFilename, "%s.prev.snapshot", filename);

                if (access(prevSnapshotFilename, F_OK) != -1) {
                    compareSnapshots(prevSnapshotFilename, snapshotFilename);
                    remove(prevSnapshotFilename);
                }

                close(fptr);

                pid_t pid = fork();

                if (pid < 0) {
                    perror("fork");
                    return;
                }

                if(pipeWrite != -1){
                    if (pid == 0) {
                        execlp("/bin/bash", "sh", "/home/ubuntu/OS/verify_for_malicious.sh", filename, NULL);

                        printf("execlp error\n");
                    } 
                }

                break;
        }
    }

    closedir(director);
}

void compareSnapshots(const char *snapshotPath1, const char *snapshotPath2) {
    FILE *file1 = fopen(snapshotPath1, "r");
    FILE *file2 = fopen(snapshotPath2, "r");

    if (file1 == NULL || file2 == NULL) {
        perror("fopen");
        return;
    }

    char line1[1024];
    char line2[1024];
    int lineCount = 0;
    int differencesFound = 0;

    while (fgets(line1, sizeof(line1), file1) != NULL && fgets(line2, sizeof(line2), file2) != NULL) {
        lineCount++;
        if (strcmp(line1, line2) != 0) {
            differencesFound = 1;
            break;
        }
    }

    if (differencesFound) {
        printf("Changes detected in %s:\n", snapshotPath2);

        rewind(file1);
        rewind(file2);

        int lineNum = 0;
        while (fgets(line1, sizeof(line1), file1) != NULL && fgets(line2, sizeof(line2), file2) != NULL) {
            lineNum++;
            if (strcmp(line1, line2) != 0) {
                printf("Line %d: ", lineNum);
                if (strlen(line1) == 0) {
                    printf("File added\n");
                } else if (strlen(line2) == 0) {
                    printf("File deleted\n");
                } else {
                    printf("File edited\n");
                }
            }
        }
    } else {
        printf("No changes detected between %s and %s\n", snapshotPath1, snapshotPath2);
    }

    fclose(file1);
    fclose(file2);
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
          
            openDirectories(argv[i], 1);
            exit(0);
        }

        int pipeFD[2];
        if (pipe(pipeFD) == -1) {
            perror("Pipe creation failed");
            return 1;
        }

        pid_t verify = fork();
        if(verify == 0){
            openDirectories(argv[i], 1);
        }
    }

    for(int i = 1; i < argc; i++){
        wait(&status);
        if(WIFEXITED(status) && pid[i-1] != 0)
            printf("Child process %d terminated with PID %d terminated with exit code %d.\n", i, pid[i-1], WEXITSTATUS(status));
    }

    return 0;
}