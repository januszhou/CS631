#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>

#include "util.h"

static void
usage(){
    fprintf(stderr, "usage : sish -x -c[command]\n");
    exit(1);
}

char*
get_input(char *buf, int bufsize){
    return fgets(buf, bufsize, stdin);
}

/**
 * if no argument, cd to user home
 * otherwise, try to cd to argu
 * 
 * @param  path user input path
 * @return      int type
 */
int change_dir(char *path) {
  char *homeDirectory;

  if (path == NULL) {
    homeDirectory = getHomeDirectory();

    if (chdir(homeDirectory) < 0) {
      fprintf(stderr, "Cannot cd into %s\n", homeDirectory);
      return -1;
    }   
  } else {
    if (chdir(path) < 0) {
      fprintf(stderr, "Cannot cd into %s\n", path);
      return -1;
    }
  }
  return 0;
}

/**
 * loop char array to output, 
 * if it has $$ or $?, we output something else
 * @param  paras  user input
 * @param  status previous proccess status
 * @return        int type
 */
int echo(char *paras[], int status){
  int i = 1;  
  while(1){
      if(paras[i] == NULL)
      {
          printf("\n");
          fflush(stdout);
          status = 0;
          return 0;
      }
      if (strncmp(paras[i], "$$", 2) == 0){
          printf("%d ", getpid());
      }else if(strncmp(paras[i], "$?", 2) == 0){
          printf("%d", status);
      }else{
          printf("%s ", paras[i]);
      }
      i++;
  }
  return 0;
}


/**
 * main execute function, 
 * @param  inputString char pointer, from user input line
 * @param  flag        either print + prefix or not
 * @param  preStatus   passing the preious process status to echo
 * @return             int type of process status return
 */
int 
execute(char *inputString, int flag, int preStatus)
{
  int mode = NORMAL, mode2 = NORMAL, status1, builtin = 0, status;
  int myPipe[2];
  char *cmdArgv[INPUT_STRING_SIZE], *supplement = NULL;
  char *cmdArgv2[INPUT_STRING_SIZE], *supplement2 = NULL;
  pid_t pid, pid2;
  char *fileLocation = NULL;
  FILE *fp;
  char *curDir = (char *)malloc(100);

  getcwd(curDir, 100);

  parse(inputString, cmdArgv, &supplement, &mode);

  if(supplement)
  {
    fileLocation = strcatTwoCharPointer(curDir, supplement);  
  }

  if(cmdArgv[0] == NULL){
    return 0;
  }

  if(flag){
    printPrefix(cmdArgv);
  }
  if(strcmp(*cmdArgv, "exit") == 0){
    builtin = 1;
    exit(0);
  }
  else if(strcmp(*cmdArgv, "cd") == 0){
    builtin = 1;
    status = change_dir(cmdArgv[1]);
  }
  else if(strcmp(*cmdArgv, "echo") == 0){
    builtin = 1;
    status = echo(cmdArgv, preStatus);
  }

  if(mode == PIPELINE)
  {
    if((status = pipe(myPipe)) < 0)
    {
      fprintf(stderr, "Pipe failed!");
    }

    parse(supplement, cmdArgv2, &supplement2, &mode2);

    if(flag){
      printPrefix(cmdArgv2);
    }
  }

  pid = fork();
  if( (status = pid) < 0)
  {
    printf("Fork error occured");
  }

  else if(pid == 0)
  {
    switch(mode)
    {
      case OUTPUT_REDIRECTION:
        if(fileLocation == NULL) break;
        fp = fopen(fileLocation, "w+");
        status = dup2(fileno(fp), 1);
        break;
      case OUTPUT_APP:
        if(fileLocation == NULL) break;
        fp = fopen(fileLocation, "a");
        status = dup2(fileno(fp), 1);
        break;
      case INPUT_REDIRECTION:
        if(fileLocation == NULL) break;
        fp = fopen(fileLocation, "r");
        status = dup2(fileno(fp), 0);
        break;
      case PIPELINE:
        close(myPipe[0]);
        status = dup2(myPipe[1], fileno(stdout));
        close(myPipe[1]);
        break;
    }
    if(builtin == 0){
      if(execvp(*cmdArgv, cmdArgv) < 0){
        status = -1;
        fprintf(stderr, "%s: %s\n", cmdArgv[0], strerror(errno));
        exit(-1);
      }
    }
  }
  else
  {
    if(mode == BACKGROUND);
    else if(mode == PIPELINE)
    {
      waitpid(pid, &status1, 0);
      pid2 = fork();
      if(pid2 < 0)
      {
        printf("Fork error occured");
        exit(-1);
      }
      else if(pid2 == 0)
      {
        close(myPipe[1]);
        dup2(myPipe[0], fileno(stdin));
        close(myPipe[0]);
        if(execvp(*cmdArgv2, cmdArgv2) < 0){
          status = -1;
          fprintf(stderr, "%s: %s\n", cmdArgv2[0], strerror(errno));
          exit(-1);
        }
      }
      else
      {
        close(myPipe[0]);
        close(myPipe[1]);
      }
    }
    else{
      waitpid(pid, &status1, 0);
    }
  }

  return status;
}

/**
 * it loop forever to get user input, pass it to execute function
 * @param flag either print prefix +
 */
void 
shell_mode(flag){
  int preStatus = 0;
  size_t len = INPUT_STRING_SIZE;
  char *inputString;
  inputString = (char*)malloc(sizeof(char)*INPUT_STRING_SIZE);

  if(!inputString){
    perror("malloc error");
    exit(1);
  }

  while(1){
    printDefault();
    getline( &inputString, &len, stdin);

    preStatus = execute(inputString, flag, preStatus);
  }

  exit(1);
}

/**
 * if -c abc, it will run the command
 * if -x, it will loop print +command line by line
 * if nothing, default going to shell_mode 
 */
int 
main(int argc, char *argv[]){
  int xflag = 0;
  char *command;
  int cp;
  int preStatus = 0;

  if(argc == 1){
    shell_mode(xflag);
  } else {
    while ((cp = getopt(argc, argv, "c:x"))!= -1) {
      switch (cp) {
        case 'x':
          xflag = 1;
          shell_mode(xflag);
          break;
        case 'c':
          command = optarg;
          execute(command, xflag, preStatus);
          break;
        default:
          usage();
          break;
      }
    }

    argv += optind;
    argc -= optind;
  }

  exit(0);
}