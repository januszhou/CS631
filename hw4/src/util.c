#define _GNU_SOURCE

#include <dirent.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "const.h"
#include "util.h"

void
getCurrentTime(char *ftime, size_t buflen) {
  time_t mtt;
  mtt = time(NULL);
  getFormattedTime(ftime, buflen, mtt, HEADER_DATE_FORMAT);

}

void
getFormattedTime(char *ftime, size_t buflen, time_t mtt, char *dformat) {
  struct tm *mt;
  setenv("TZ", "GMT", 1);
  tzset();
  mt = localtime(&mtt);
  char *format;
  format = strncmp(dformat,DATE_FORMAT,strlen(DATE_FORMAT)) == 0 ? "%F %H:%M:%S" : "%a, %d %b %Y %T %Z";
  strftime(ftime,buflen,format,mt);

}

int
parseHttpDate(char *http_date, time_t *tms) {

  char *remainder;
  struct tm tm;
  memset(&tm, 0 , sizeof(struct tm));

  if ((remainder = strptime(http_date, "%a, %d %b %Y %T GMT", &tm)) == NULL ) {
    return 0;
  }

  /* If the input string contains more characters than required by format string the return value of strptime() 
   * points right after the last consumed input character. 
   */
  if (*remainder)
    return 0; 

  *tms = timegm(&tm);
  return 1;

}

void
URIDecode(char * instr, char* outstr) {
  int sz;
  sz=strlen(instr);
  int i;
  int j=0;
  for (i=0; i<sz; ++i) {
    if(instr[i]=='%' && i< sz-2) {
      char tmp ;
      tmp = instr[i+3];
      instr[i+3]=0;
      outstr[j]=(char)strtol(instr+i+1,NULL,16);
      instr[i+3]=tmp;
      i=i+2;
    } else {
      outstr[j]=instr[i];
    }
    j++;
  }
  outstr[j]=0;
}

/* Writes log data to a file descriptor passed to program. Takes the values in struct logging and 
 * writes to fd in the and format provided in the man page.
 */
int
writelog(int fd, struct logging* log)
{
  char logstr[sizeof(struct logging) + 16];
  int n = 0;
  snprintf(logstr, sizeof(logstr), "=> %s [%s] \"%s\" %s %s\n", log->remoteip,
      log->request_time, log->request_lineq, log->request_status,
      log->response_size);
  if((n = write(fd, logstr, strlen(logstr))) < 0) {
    perror("Write error");
    exit(EXIT_FAILURE);
  }
  return 0;
}

void init_logging(struct logging* l)
{
  bzero(l->remoteip, sizeof(l->remoteip));
  bzero(l->request_time, sizeof(l->request_time));
  bzero(l->request_lineq, sizeof(l->request_lineq));
  bzero(l->request_status, sizeof(l->request_status));
  bzero(l->response_size, sizeof(l->response_size));
}
