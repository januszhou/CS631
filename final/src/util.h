#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGRAM_NAME "sish"
#define PREFIX_CHAR "+"
#define INPUT_STRING_SIZE 256

#define NORMAL 				0
#define OUTPUT_REDIRECTION 	1
#define INPUT_REDIRECTION 	2
#define PIPELINE 			3
#define BACKGROUND			4
#define OUTPUT_APP	5

char* getHomeDirectory();
void printDefault(void);
int parse(char *, char *[], char **, int *);
void chop(char *);
void printPrefix(char *[]);
int checkCharEmpty(char *);
char* strcatTwoCharPointer(char *, char *);