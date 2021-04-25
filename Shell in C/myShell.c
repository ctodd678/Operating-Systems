#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/stat.h>

//ENVIRONMENT VARIABLES w/ DEFAULT SETTINGS
char PATH[256] = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:$HOME/bin/\0";
char HISTFILE[64];
char HOME[64] = "./";

struct pidTracker{
  pid_t pid;
  int n;
  char *arg;
}typedef pidTracker;

char         *readLine              ();
char        **getLineArgs           (char *);
int           execCMD               (char **, pidTracker **);
int           exitCMD               ();
void          lsCMD                 (char **);
void          helpCMD               ();
int           countArgs             (char **);
void          backgroundHandler     (int); 
char          checkAll              (char **);
int           checkOut              (char **);
int           checkIn               (char **);
char        **checkPiped            (char **);
int           execPiped             (char **, char **, pidTracker **);
int           execBackground        (char **, int*);
int           optionHandler         (char **, pidTracker **, FILE *);
int           countPid              (pidTracker **);
int           findEmptyPid          (pidTracker **);
pidTracker  **initPidTracker        ();
int           historyCMD            (FILE *, char **);
int           lineCount             (FILE *);
int           changedirCMD          (char **);
int           exportCMD             (char **);
int           echoCMD              (char **);
char         *findPATH              (char **);


int main(int argc, char **argv) {

  char *line, **args, directory[256];
  int i = 0, status, end = 1, loopCount = 0;
  pidTracker **pidT;
  FILE *profFp;
  FILE *histFp;

  profFp = fopen(".CIS3110_profile", "w");
  histFp = fopen(HISTFILE, "a+");
  fseek(histFp, 0, SEEK_SET);

  //SETUP SIGACTION
  struct sigaction sigact;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_handler = backgroundHandler;

  if (sigaction(SIGQUIT, &sigact, NULL) < 0) {
    perror("sigaction()");
    exit(1);
  }

  pidT = initPidTracker();  //initialize pidTracker structure

  getcwd(directory, sizeof(directory)); //get user name and current directory

  //Run loop for user input on command line
  while(1){
    i = 0;
    while(i < 100) {
      if(pidT[i]->pid != 0) {
        if(waitpid(pidT[i]->pid, &status, WNOHANG) == pidT[i]->pid){
          printf("\n[%d]+  Done(%d)     %s " , i + 1, pidT[i]->pid, pidT[i]->arg);
          pidT[i]->pid = (pid_t) 0;
          pidT[i]->arg = NULL;
          free(pidT[i]->arg);
        }
      }
      i++;
    }
    
    printf("\n%s:%s$ " , getenv("USER"), directory); //print current user and directory
    line = readLine();

    fprintf(histFp, "%s", line);
    fseek(histFp, 0, SEEK_END);
    fflush(histFp);

    args = getLineArgs(line);

    end = optionHandler(args, pidT, histFp);

    getcwd(directory, sizeof(directory));
    free(line);
    free(args);
    loopCount++;

    if(end == 0){
      break;
    }
  }

  for(i = 0; i < 100; i++) {
    free(pidT[i]);
  }
  free(pidT);

  exitCMD();
  printf("\n");
  return 0;
}

char *readLine() {
  
  char *currLine = NULL;
  size_t buffer = 0;

  //read current line
  if(getline(&currLine, &buffer, stdin) == -1){
    if(feof(stdin)) {
      exit(0);
    }
    else {
      fprintf(stderr, "ERROR! Cannot read line in readLine.");
      exit(1);
    }
  }
  return currLine;
}

char **getLineArgs(char *line) {

  int i = 0;
  char **args;
  char *currArg;

  //allocate memory for array of argument strings
  if (!(args = malloc(sizeof(char*) * 1024))) {
    fprintf(stderr, "ERROR! Failed malloc in getLineArgs.\n");
    exit(EXIT_FAILURE);
  }

  currArg = strtok(line, " ");                  //split line 
  while (currArg != NULL) {
    args[i] = currArg;                          //set currArg to nth argument
    i++;

    currArg = strtok(NULL, " ");                //set currArg to next argument
  }
  args[i - 1][strlen(args[i - 1]) - 1] = '\0';  //remove new line character from last argument
  args[i] = NULL;                               //set last argument to NULL

  return args;                                  //return array of argument strings
}

int optionHandler(char **args, struct pidTracker **pidT, FILE *hist) {
  char o = 'o';
  int end = 1;
  int argCount = countArgs(args);
  o = checkAll(args);

  //CHECK FOR CMDS
  if (strcmp(args[0], "exit") == 0) {
    return 0;
  }
  else if (strcmp(args[0], "help") == 0) {
  	helpCMD();
    return 1;
  }
  else if (strcmp(args[0], "history") == 0) {
  	historyCMD(hist, args);
    return 1;
  }
  else if (strcmp(args[0], "cd") == 0) {
  	changedirCMD(args);
    return 1;
  }
  else if (strcmp(args[0], "export") == 0) {
  	exportCMD(args);
    return 1;
  }
  else if (strcmp(args[0], "echo") == 0) {
  	echoCMD(args);
    return 1;
  }
  else if ((strcmp(args[0], "\0") == 0) && argCount <= 1) {
    return 1;
  }
  else if(o == '|') {
    char **pipedArgs;                 //create second string for piped arguments
    pipedArgs = checkPiped(args);     //get piped arguments
    execPiped(args, pipedArgs, pidT); //exec pipe
    end = 1;
    free(pipedArgs);                  //free the string with piped arguments
  }
  else {
    end = execCMD(args, pidT);        //exec cmd
  }

  return end;
}

int execCMD(char **args, struct pidTracker ** pidT) {
  pid_t cpid;
  int status;
  int argCount = 0;
  int pidI = 0;
  int i;
  int in = checkIn(args);
  int out = checkOut(args);
  int backGrnd = 0;

  argCount = countArgs(args);

  if(strcmp(args[argCount - 1], "&") == 0){
    backGrnd = 1;
    args[argCount - 1] = NULL;
  }

  FILE *fp, *fp2;

  if (strcmp(args[0], "exit") == 0) {
  	return 0;
  }
  if (strcmp(args[0], "help") == 0) {
  	helpCMD();
    return 1;
  }
  if ((strcmp(args[0], "\0") == 0) && argCount <= 1) {
    return 1;
  }

  
  
  cpid = fork();
  //CHILD PROCESS
  if (cpid == 0) {
    //INPUT AND OUTPUT REDIRECTION
    if(out > 0 && in > 0) {
      if(in < out) {
        
        args[in - 1] = NULL;
        fp = freopen(args[in], "r", stdin);
        args[out - 1] = NULL;
        fp2 = freopen(args[out], "w+" , stdout);
        if(execvp(args[0], args) < 0) {
          if(args[0][0] == '.')
            printf("-myShell: %s: No such file or directory" , args[0]);
          else
            printf("-myShell: %s: command not found" , args[0]);
        }
        fclose(fp);
        fclose(fp2);
      }
      else if(in > out) {
        args[out - 1] = NULL;
        fp2 = freopen(args[out], "w+" , stdout);
        args[in - 1] = NULL;
        fp = freopen(args[in], "r", stdin);
        if(execvp(args[0], args) < 0) {
          if(args[0][0] == '.')
            printf("-myShell: %s: No such file or directory" , args[0]);
          else
            printf("-myShell: %s: command not found" , args[0]);
        }
        fclose(fp);
        fclose(fp2);
      }
    }
    else if(out > 0) {
      args[out - 1] = NULL;
      fp = freopen(args[out], "w+", stdout);
      if(execvp(args[0], args) < 0) {
        if(args[0][0] == '.')
          printf("-myShell: %s: No such file or directory" , args[0]);
        else
          printf("-myShell: %s: command not found" , args[0]);
        fclose(fp);
        return 1;
      }
      fclose(fp);
      return 1;
    }
    //INPUT REDIRECTION
    else if(in > 0) {
      args[in - 1] = NULL;
      fp = freopen(args[in], "r", stdin);
      if(execvp(args[0], args) < 0) {
        if(args[0][0] == '.')
          printf("-myShell: %s: No such file or directory" , args[0]);
        else
          printf("-myShell: %s: command not found" , args[0]);
        fclose(fp);
        return 1;
      }
      fclose(fp);
      return 1;
    }
    //NO FILE SPECIFIED FOR REDIRECTION
    else if((in == -2) || (out == -2)) {
        printf("\nERROR! No input/output file specified!");
        return 1;
    }
    //EXECUTING BASH COMMANDS
    else if(execvp(args[0], args) < 0) {
      if(args[0][0] == '.')
        printf("-myShell: %s: No such file or directory" , args[0]);
      else
        printf("-myShell: %s: command not found" , args[0]);
      return 1;
    }
  }
  //FORK ERROR HANDLING 
  else if (cpid == -1) {
      printf("ERROR! Failed fork in execCMD.");
      return 1;
  }
  //PARENT PROCESS 
  else {
    if(backGrnd == 1) {
      pidI = findEmptyPid(pidT);
      pidT[pidI]->pid = cpid;
      pidT[pidI]->n = 0;
      i = 0;
      pidT[pidI]->arg = malloc(sizeof(char*) * 1024);
      strcpy(pidT[pidI]->arg, " ");
      while(args[i] != NULL) {
         strcat(pidT[pidI]->arg, args[i]);
         strcat(pidT[pidI]->arg, " ");
         i++;
      }
      printf("[%d] %d" , pidI + 1, cpid);
    }
    else {
       waitpid(cpid, &status, WUNTRACED);
    }
  }

  return 1;
}

int execPiped(char **args, char **pipedArgs, struct pidTracker **pidT) {

  pid_t cpid1, cpid2;
  int piped[2];
  int status1, status2;
  int argCount = 0;
  int backGrnd = 0;
  int inArgs = checkIn(args);
  int outArgs = checkOut(args);
  int inPipe = checkIn(pipedArgs);
  int outPipe = checkOut(pipedArgs);
  int i = 0, pidI = 0, pidI2 = 0;
  FILE *fp, *fp2;
  
  argCount = countArgs(pipedArgs);

  if(strcmp(pipedArgs[argCount - 1], "&") == 0){
    backGrnd = 1;
    pipedArgs[argCount - 1] = NULL;
  }
  if ((strcmp(args[0], "\0") == 0) && argCount <= 1) {
    return 1;
  }
  if(pipe(piped) < 0) {
    printf("\nERROR! Cannot pipe in execPiped.");
    return 1;
  }

  cpid1 = fork();
  if (cpid1 == 0) {
    
    dup2(piped[1], 1);
    close(piped[1]);
    close(piped[0]);

    if(inArgs > 0) {
      args[inArgs - 1] = NULL;
      fp = freopen(args[inArgs], "r", stdin);
    }

    if(execvp(args[0], args) < 0) {
        printf("\nERROR! Executing command(1) in execPiped.");
        return 1;
    }
    fclose(fp);
  }
  else if (cpid1 == -1) {
      printf("\nERROR! Failed fork in execCMD.");
      return 1;
  } 
  else {
    cpid2 = fork();
    if (cpid2 == 0) {
      close(piped[1]);
      dup2(piped[0], 0);
      close(piped[0]);

      if(outPipe > 0) {
        pipedArgs[outPipe - 1] = NULL;
        fp2 = freopen(pipedArgs[outPipe], "w+", stdout);
      }

      if(execvp(pipedArgs[0], pipedArgs) < 0) {
        printf("\nERROR! Executing command(1) in execPiped.");
        return 1;
      }
      fclose(fp2);
    }
    else if (cpid2 == -1) {
      printf("\nERROR! Failed fork in execCMD.");
      return 1;
    }
    close(piped[0]); 
    close(piped[1]);

    //BACKGROUND PIPE
    if(backGrnd == 1) {
      pidI = findEmptyPid(pidT);
      pidT[pidI]->pid = cpid1;
      pidT[pidI]->n = 0;
      i = 0;
      pidT[pidI]->arg = malloc(sizeof(char) * 1024);
      strcpy(pidT[pidI]->arg, " ");
      
      while(args[i] != NULL) {
         strcat(pidT[pidI]->arg, args[i]);
         strcat(pidT[pidI]->arg, " ");
         i++;
      }

      pidI2 = findEmptyPid(pidT);
      pidT[pidI2]->pid = cpid2;
      pidT[pidI2]->n = 0;
      i = 0;
      pidT[pidI2]->arg = malloc(sizeof(char) * 1024);
      strcpy(pidT[pidI2]->arg, " ");
      while(pipedArgs[i] != NULL) {
         strcat(pidT[pidI2]->arg, pipedArgs[i]);
         strcat(pidT[pidI2]->arg, " ");
         i++;
      }

      printf("[%d] %d" , pidI + 1, cpid1);
      printf("  [%d] %d" , pidI2 + 1, cpid2);
    }
    else {
      waitpid(cpid1, &status1, 0);
      waitpid(cpid2, &status2, 0);
    } 
  }
  return 1;
}

void backgroundHandler(int signo) {
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0);
}

int exitCMD() {
  printf("myShell terminating...\n");
  exit(1);
}

void helpCMD() {
  printf("myShell version 0.01, release 2021");
  printf("\nList of commands in shell:");
  printf("\ncd");
  printf("\nexit");
  printf("\nls\n");
}

char checkAll(char **args) {
  int i = 0;
  while(args[i] != NULL) {
    if(strcmp(args[i], "|") == 0) {
      return '|'; //return option
    }
    else if(strcmp(args[i], "&") == 0) {
      return '&';     //return option
    }
    
    
    i++;
  }
  return '\0';  //return null character if no option found
}

int checkOut(char **args) {
  int i = 0;
  while(args[i] != NULL) {
    if(strcmp(args[i], ">") == 0) {
      if(args[i + 1] != NULL) {
          return i + 1;   //return index of file name
      }
      else {
        return -2;
      }
    }
    i++;
  }
  return -1;
}

int checkIn(char **args) {
  int i = 0;
  while(args[i] != NULL) {
    if(strcmp(args[i], "<") == 0) {
      if(args[i + 1] != NULL) {
          return i + 1;   //return index of file name
      }
      else {
        return -2;
      }
    }
    i++;
  }
  return -1;
}

char **checkPiped(char **args) {
  int i = 0;
  int j = 0;
  char **piped;
  //char  *currPipe;
  int pipeI = 0;
  if (!(piped = malloc(sizeof(char*) * 1024))) {
    fprintf(stderr, "ERROR! Failed malloc in getLineArgs.\n");
    exit(EXIT_FAILURE);
  }
  
  i = 0;
  //GET ONLY PIPED ARGS IN PIPED
  while(args[i] != NULL) {
    if(strcmp(args[i], "|") == 0) {
      pipeI = i;
      i++;
      while(args[i] != NULL){
        //strcpy(piped[j], args[i]);
        //memcpy(&piped[j], &args[i], sizeof(char*));
        piped[j] = args[i];
        j++;
        i++;
      }
      piped[j] = NULL;    // set last to NULL
      args[pipeI] = NULL; // set | symbol to null
      break;
    }
    i++;
  }
  
  return piped;
}

int countArgs(char **args) {
  int i = 0;
  while(args[i] != NULL) {
    i++;
  }

  return i;
}

pidTracker **initPidTracker() {

  pidTracker **temp = malloc(sizeof(pidTracker*) * 100);
  int i;

  for(i = 0; i < 100; i++){
    temp[i] = malloc(sizeof(pidTracker) * (i + 1));
    temp[i]->pid = 0;
    temp[i]->n = 0;
    temp[i]->arg = NULL;
  }

  return temp;
}

int countPid(struct pidTracker** pidT) {

  int i = 0;
  int count = 0;
  for(i = 0; i < 100; i++) {
    if(pidT[i]->pid != 0) {
      count++;
    }
  }
  return count;
}

int findEmptyPid(struct pidTracker** pidT) {

  int i = 0;
  for(i = 0; i < 100; i++) {
    if(pidT[i]->pid == (pid_t) 0) {
      return i;
      break;
    }
  }
  return i;
}

int changedirCMD(char **args) {
  if(args[1] != NULL) {
    chdir(args[1]);
    return 1;
  }
  return 1;
}

int exportCMD(char **args) {
  char *c = args[1];
  int i = 0;
  if (c == NULL) {
    printf("\n$PATH=%s", PATH);
    printf("\n$HISTFILE=%s", HISTFILE);
    printf("\n$HOME=%s", HOME);
  }
  else if(c != NULL) {
    if(c[0] == 'P' && c[1] == 'A') {

      while(c[i + 5] != '\0') {
        PATH[i] = c[i + 5];
        i++;
      }
      PATH[i] = '\0';
    }
    else if(c[0] == 'H' && c[1] == 'I') {
      while(c[i + 9] != '\0') {
        HISTFILE[i] = c[i + 9];
        i++;
      }
      HISTFILE[i] = '\0';
    }
    else if(c[0] == 'H' && c[1] == 'O') {
      while(c[i + 5] != '\0') {
        HOME[i] = c[i + 5];
        i++;
      }
      HOME[i] = '\0';
    }
  }
}

int echoCMD(char **args) {
  char *c = args[1];
  int i = 0;
  if (c == NULL) {
    printf("\nPlease enter an argument.");
  }
  else if(c != NULL) {
    if(c[1] == 'P' && c[2] == 'A') {
      printf("%s\n", PATH);
    }
    else if(c[1] == 'H' && c[2] == 'I') {
      printf("%s\n", HISTFILE);
    }
    else if(c[1] == 'H' && c[2] == 'O') {
      printf("%s\n", HOME);
    }
  }
}

int historyCMD(FILE *fp, char **args) {

  int i = 1, n = 0;
  int lines = 0;
  char currLine[1024];

  if(args[2] != NULL) {
    perror("\nERROR! More than 2 arguments entered for history.");
    return 1;
  }
  else if(args[1] == NULL) {
    fclose(fp);
    fp = fopen(HISTFILE, "r");
    while(fgets(currLine, sizeof(currLine), fp) != NULL) {     
      printf(" %d  %s" , i, currLine);
      i++;
    }
    fclose(fp);
    fp = fopen(HISTFILE, "a+");
    return 1;
  }
  else if(strcmp(args[1], "-c") == 0) {
    fp = fopen(HISTFILE, "w+"); //overwrite file
    fclose(fp);
    fp = fopen(HISTFILE, "a+"); //open file for appending
    return 1; 
  }
  else if(atoi(args[1]) > 0) {
    n = atoi(args[1]);                    //get int from string
    i = 1;
    fclose(fp);
    fp = fopen(HISTFILE, "r");
    lines = lineCount(fp);
    fseek(fp, 0, SEEK_SET);
    while(fgets(currLine, sizeof(currLine), fp) != NULL) {
      if(i > (lines - n)){
        printf(" %d  %s" , i, currLine);
      }
      i++;
    }
    fclose(fp);
    fp = fopen(HISTFILE, "a+");
    return 1;
  }

  return 1;
}

int lineCount(FILE *fp) {

  char c;
  int count = 0;

  while(!(feof(fp))) {
    c = fgetc(fp);
    if(c == '\n') {
      count++;
    }
  }

  return count;
}

char *findPATH(char **args) {

  char **paths;
  char  *currPath;
  int i = 0;
  int pathCount = 0;

  //allocate memory for array of argument strings
  if (!(paths = malloc(sizeof(char*) * 128))) {
    fprintf(stderr, "ERROR! Failed malloc in getLineArgs.\n");
    exit(EXIT_FAILURE);
  }

  //allocate memory for array of argument strings
  if (!(currPath = malloc(sizeof(char) * 256))) {
    fprintf(stderr, "ERROR! Failed malloc in getLineArgs.\n");
    exit(EXIT_FAILURE);
  }
 
  currPath = strtok(PATH, ":");                  //split line 
  while (currPath != NULL) {
    paths[i] = malloc(sizeof(currPath) + 24);    //malloc with extra space for args
    strcpy(paths[i], currPath);                  //set currArg to nth argument
    //strcpy(paths[i], currPath);
    i++;
    currPath = strtok(NULL, ":");                //set currArg to next argument
  }
  pathCount = i;
  paths[i - 1][strlen(paths[i - 1]) - 1] = '\0'; //remove new line character from last argument
  paths[i] = NULL;                               //set last argument to NULL


  i = 0;
  while(paths[i] != NULL) {
    strcat(paths[i], "/");
    strcat(paths[i], args[0]);
    //sprintf(paths[i], "%s/ls", paths[i]);
    printf("%s\n" , paths[i]);
    i++;
  }
  i = 0;
  struct stat *buf;
  buf = malloc(sizeof(struct stat));
  //sprintf(paths[0], "%s/ls", paths[0]);
  //printf("\n%s" , paths[0]);
  //printf("\nSTAT: %d\n" , stat(paths[0], buf));

  while(paths[i] != NULL) {
    if(stat(paths[0], buf) == 0) {
      currPath = realloc(currPath, sizeof(paths[i]) + 1);
      strcpy(currPath, paths[i]);
      return currPath;
    }
    i++;
  }

  for(i = 0; i < pathCount; i++) {
    free(paths[i]);
  }
  free(paths);
  free(buf);

  return NULL;      
}