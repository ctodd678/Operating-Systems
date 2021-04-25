#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

#define MAXDIRS 1024

DIR *dp;
struct dirent *dir;

int     getNumDirs          (char *);
int     getNumFiles         (char *);
void    listdir             (const char *, int, int, int);
void    printFilesTree      (const char *);
void    printDirsTree       (const char *);
void    printFilesInode     (const char *);
void    printDirsInode      (const char *);
char*   getPerms            (char *);

//File System Report (Connor Toddd)

int main(int argc, char *argv[]) {

    FILE *fp;
    int tree = 0;
    int inode = 0;
    char rootDir[256];
    struct stat buffer;
    int status, numFiles, numDirs;
    int level;
    int maxLevel = 0;
    numFiles = 0, numDirs = 0, level = 1;


    //*INPUT ERROR HANDLING
    if(argc > 3) {
        printf("ERROR! Too many arguments entered!\n");
        printf("Example args: './FSreport -tree /home/myname/rootDir'\n");
        printf("Exiting...\n");
        exit(1);
    }
    else if(argc < 3) {
        printf("ERROR! Not enough arguments entered!\n");
        printf("Example args: './FSreport -tree /home/myname/rootDir'\n");
        printf("Exiting...\n");
        exit(1);
    }

    //*CHECK MODE
    if(strcmp(argv[argc - 2], "-tree") == 0) {
        tree = 1;
    }
    else if(strcmp(argv[argc - 2], "-inode") == 0) {
        inode = 1;
    }

    strcpy(rootDir, "");
    strcpy(rootDir, argv[argc - 1]);
    //printf("TREE: %d    INODE: %d   DIR: %s\n", tree, inode, argv[argc - 1]);

    status = stat(rootDir, &buffer);
    //printf("STATUS: %d\n", status);

    if(status != -1) {

        if(tree) {
             printf("File System Report: Tree Directory Structure\n\n");

            //*PRINT FIRST LEVEL
            int nDirs = getNumDirs(rootDir);
            int nFils = getNumFiles(rootDir);
            printf("Level 1: %s\n", rootDir);
            if(nDirs > 0) {
                printf("Directories\n");
                printDirsTree(rootDir);
            }
            if(nFils > 0) {
                if(nDirs > 0) printf("\n");
                printf("Files\n");
                printFilesTree(rootDir);
            }

            //*PRINT THE REST
            for(int i = 1; i <= 128; i++) {
                listdir(rootDir, 0, 1, i);  //*MODE 0 FOR TREE FORMAT
            }
        }
        else if(inode) {
            printf("File System Report: Inodes\n\n");

            //*PRINT FIRST LEVEL
            int nDirs = getNumDirs(rootDir);
            int nFils = getNumFiles(rootDir);
            printf("Level 1 Inodes: %s\n", rootDir);
            if(nDirs > 0) {
                printDirsInode(rootDir);
            }
            if(nFils > 0) {
                printFilesInode(rootDir);
            }

            //*PRINT THE REST
            for(int i = 1; i <= 128; i++) {
                listdir(rootDir, 1, 1, i);  //*MODE 1 FOR INDOE FORMAT
            }
        }
    }
    else {
        printf("Invalid directory entered!\n");
        printf("Please run the program again with a valid directory path.\n");
    }

    printf("\n\nExiting...\n");
    return 0;
}


//*FUNCTIONS

void listdir(const char *path, int mode, int lvl, int lvlToPrint) {

    DIR *dir;
    struct dirent *currentDir;
    char sub_dir[1024];

    if (!(dir = opendir(path))) {
        return;
    }

    while ((currentDir = readdir(dir)) != NULL) {
        if (currentDir->d_type == DT_DIR) {
            if (strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
                continue;
            
            sprintf(sub_dir, "%s/%s", path, currentDir->d_name);

            if(lvl == lvlToPrint) {
                printf("\nLevel %d: %s\n", lvl + 1, currentDir->d_name);
                int nDirs = getNumDirs(sub_dir);
                int nFils = getNumFiles(sub_dir);
                if(nDirs > 0) {
                    if(mode == 0) {
                        printf("Directories\n");
                        printDirsTree(sub_dir);
                    }
                    else {
                        printDirsInode(sub_dir);
                    }
                }
                if(nFils > 0) {
                    if(mode == 0) {
                        if(nDirs > 0) printf("\n");
                        printf("Files\n");
                        printFilesTree(sub_dir);
                    }
                    else {
                        printFilesInode(sub_dir);
                    }
                }
            }
            listdir(sub_dir, mode, lvl + 1, lvlToPrint);
        }
    }

    closedir(dir);
}

void printFilesTree(const char *path) {

    DIR *dir;
    struct dirent *currentDir;
    char filePath[1024];

    if (!(dir = opendir(path))) {
        return;
    }
   
    while ((currentDir = readdir(dir)) != NULL) {
        if (strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0 || strstr(currentDir->d_name, ".ini") != NULL)
            continue;
        if(currentDir->d_type != DT_DIR) {
            struct stat buffer;
            struct passwd  *pwd;
            struct group   *grp;
            char *last_access;
            char *otherTime;
            sprintf(filePath, "%s/%s", path, currentDir->d_name);

            int status = stat(filePath, &buffer);
            pwd = getpwuid(buffer.st_uid);
            grp = getgrgid(buffer.st_gid);
            last_access = ctime((time_t*)&buffer.st_atim);

            if(&buffer.st_mtim > &buffer.st_ctim) 
                otherTime =  ctime((time_t*)&buffer.st_ctim);
            else
                otherTime =  ctime((time_t*)&buffer.st_mtim);

            printf("%-8.8s(%-8.8s) \t",pwd->pw_name, grp->gr_name);  //* OWNER NAME (GROUP NAME)
            printf("%ld \t", buffer.st_ino);                         //* INODE NUMBER
            printf("%s \t", getPerms(filePath));                     //* PERMISSIONS
            printf("%ld \t", buffer.st_size);                        //* SIZE
            printf("%s \n", currentDir->d_name);                            //* FILE NAME
            printf("\t%s", last_access);                             //* LAST ACCESS TIME
            printf("\t%s\n", otherTime);                             //* OTHER TIME
        }
    }
    closedir(dir);
}

void printDirsTree(const char *path) {

    DIR *dir;
    struct dirent *currentDir;
    char dirPath[1024];

    if (!(dir = opendir(path))) {
        return;
    }

    while ((currentDir = readdir(dir)) != NULL) {
        if (strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
            continue;

        if(currentDir->d_type == DT_DIR) {
            struct stat buffer;
            struct passwd  *pwd;
            struct group   *grp;
            char *last_access;
            char *otherTime;
            
            sprintf(dirPath, "%s/%s", path, currentDir->d_name);

            int status = stat(dirPath, &buffer);
            pwd = getpwuid(buffer.st_uid);
            grp = getgrgid(buffer.st_gid);
            last_access = ctime((time_t*)&buffer.st_atim);

            if(&buffer.st_mtim > &buffer.st_ctim) 
                otherTime =  ctime((time_t*)&buffer.st_ctim);
            else
                otherTime =  ctime((time_t*)&buffer.st_mtim);


            printf("%-8.8s(%-8.8s) \t",pwd->pw_name, grp->gr_name);  //* OWNER NAME (GROUP NAME)
            printf("%ld \t", buffer.st_ino);                         //* INODE NUMBER
            printf("%s \t", getPerms(dirPath));                      //* PERMISSIONS
            printf("%ld \t", buffer.st_size);                        //* SIZE
            printf("%s \n", currentDir->d_name);                     //* DIR NAME
            printf("\t%s", last_access);                             //* LAST ACCESS TIME
            printf("\t%s\n", otherTime);                             //* OTHER TIME

        }
    }

    closedir(dir);

}

void printFilesInode(const char *path) {

    DIR *dir;
    struct dirent *currentDir;
    char filePath[1024];

    if (!(dir = opendir(path))) {
        return;
    }
  
    while ((currentDir = readdir(dir)) != NULL) {
        if (strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
            continue;
        if(currentDir->d_type != DT_DIR) {

            struct stat buffer;
            sprintf(filePath, "%s/%s", path, currentDir->d_name);
            int status = stat(filePath, &buffer);

            printf("%d: \t", (int)buffer.st_ino);               //* INODE NUMBER
            printf("%d \t", (int)buffer.st_size);               //* SIZE
            printf("%d \t", (int)buffer.st_blocks);             //* 512 BLOCKS
            printf("%d \t", (int)(buffer.st_size / 512));         //* SIZE / 512
            printf("%s \n", currentDir->d_name);                   //* FILE NAME
        }
    }

    closedir(dir);
}

void printDirsInode(const char *path) {

    DIR *dir;
    struct dirent *currentDir;
    char filePath[1024];

    if (!(dir = opendir(path))) {
        return;
    }
  
    while ((currentDir = readdir(dir)) != NULL) {
        if (strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
            continue;
        if(currentDir->d_type == DT_DIR) {

            struct stat buffer;
            sprintf(filePath, "%s/%s", path, currentDir->d_name);
            int status = stat(filePath, &buffer);

            printf("%d: \t", (int)buffer.st_ino);               //* INODE NUMBER
            printf("%d \t", (int)buffer.st_size);               //* SIZE
            printf("%d \t", (int)buffer.st_blocks);             //* 512 BLOCKS
            printf("%d \t", (int)(buffer.st_size / 512));         //* SIZE / 512
            printf("%s \n", currentDir->d_name);                   //* FILE NAME
        }
    }

    closedir(dir);
}

char* getPerms(char *file){

    struct stat buffer;
    char *permString = malloc(sizeof(char) * 9 + 1);

    if(stat(file, &buffer) == 0){
        mode_t p = buffer.st_mode;
        permString[0] = (p & S_IRUSR) ? 'r' : '-';
        permString[1] = (p & S_IWUSR) ? 'w' : '-';
        permString[2] = (p & S_IXUSR) ? 'x' : '-';
        permString[3] = (p & S_IRGRP) ? 'r' : '-';
        permString[4] = (p & S_IWGRP) ? 'w' : '-';
        permString[5] = (p & S_IXGRP) ? 'x' : '-';
        permString[6] = (p & S_IROTH) ? 'r' : '-';
        permString[7] = (p & S_IWOTH) ? 'w' : '-';
        permString[8] = (p & S_IXOTH) ? 'x' : '-';
        permString[9] = '\0'; 
    }
    else{
        free(permString);
        return NULL;
    }   

    return permString;    
}

int getNumDirs(char* path) {

    DIR* dir;
    struct dirent* currentDir;
    int numDirs = 0;
    

    if (!(dir = opendir(path))) {
        return -1;
    }

    while((currentDir = readdir(dir)) != NULL) {
        struct stat st;

        if(strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
            continue;
    
        if (fstatat(dirfd(dir), currentDir->d_name, &st, 0) < 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            numDirs++;
        } 
    }

    closedir(dir);

    return numDirs;
}

int getNumFiles(char* path) {

    DIR* dir;
    struct dirent* currentDir;
    int numFiles = 0;
    
    if (!(dir = opendir(path))) {
        return -1;
    }

    while((currentDir = readdir(dir)) != NULL) {
        struct stat st;

        if(strcmp(currentDir->d_name, ".") == 0 || strcmp(currentDir->d_name, "..") == 0)
            continue;
    
        if (fstatat(dirfd(dir), currentDir->d_name, &st, 0) < 0) {
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            numFiles++;
        } 
    }

    closedir(dir);

    return numFiles;
}