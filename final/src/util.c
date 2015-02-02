#include "util.h"

char* 
getHomeDirectory(){
  struct passwd *pw = getpwuid(getuid());

  return pw->pw_dir;
}

void 
printDefault(void) 
{
  printf("%s$ ", PROGRAM_NAME);
}

int 
parse(char *inputString, char *cmdArgv[], char **supplementPtr, int *modePtr)
{
  int cmdArgc = 0;
  int terminate = 0;
  char *srcPtr = inputString;
    //printf("parse fun%sends", inputString);
  while(*srcPtr != '\0' && *srcPtr != '\n' && terminate == 0)
  {
    *cmdArgv = srcPtr;
    cmdArgc++;
        //printf("parse fun2%sends", *cmdArgv);
    while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\0' && *srcPtr != '\n' && terminate == 0)
    {
      switch(*srcPtr)
      {
        case '&':
        *modePtr = BACKGROUND;
        break;
        case '>':
        *modePtr = OUTPUT_REDIRECTION;
        *cmdArgv = '\0';
        srcPtr++;
                    /* implement >> */
        if(*srcPtr == '>')
        {
          *modePtr = OUTPUT_APP;
          srcPtr++;
        }
        while(*srcPtr == ' ' || *srcPtr == '\t')
          srcPtr++;
        *supplementPtr = srcPtr;
        chop(*supplementPtr);
        terminate = 1;
        break;
        case '<':
        *modePtr = INPUT_REDIRECTION;
        *cmdArgv = '\0';
        srcPtr++;
        while(*srcPtr == ' ' || *srcPtr == '\t')
          srcPtr++;
        *supplementPtr = srcPtr;
        chop(*supplementPtr);
        terminate = 1;
        break;
        case '|':
        *modePtr = PIPELINE;
        *cmdArgv = '\0';
        srcPtr++;
        while(*srcPtr == ' ' || *srcPtr == '\t')
          srcPtr++;
        *supplementPtr = srcPtr;
        terminate = 1;
        break;
      }
      srcPtr++;
    }
    while((*srcPtr == ' ' || *srcPtr == '\t' || *srcPtr == '\n') && terminate == 0)
    {
      *srcPtr = '\0';
      srcPtr++;
    }
    cmdArgv++;
  }
  *cmdArgv = '\0';
  return cmdArgc;
}

void chop(char *srcPtr){
  while(*srcPtr != ' ' && *srcPtr != '\t' && *srcPtr != '\n')
  {
    srcPtr++;
  }
  *srcPtr = '\0';
}

void printPrefix(char *cmdArgv[]){
  int i = 0;
  while(1){
    if(cmdArgv[i] == NULL){
      return;
    }
    printf("%s%s\n", PREFIX_CHAR, cmdArgv[i]);
    i++;
  }
}

char* strcatTwoCharPointer(char *str1, char *str2){
  char *slash = "/";
  char* str3 = (char *)malloc(1 + strlen(str1)+ strlen(str2)+ strlen(slash));
  strcpy(str3, str1);
  strcat(str3, slash);
  strcat(str3, str2);
  return str3;
}