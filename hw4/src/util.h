#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <dirent.h>

#define BUF_SIZE (4 * 1024)

struct logging
{
  char remoteip[64];
  char request_time[128];
  char request_lineq[BUF_SIZE];
  char request_status[16];
  char response_size[64];
};

void getFormattedTime(char *, size_t, time_t, char *);
void getCurrentTime(char *, size_t);
void URIDecode(char * , char *);
int parseHttpDate(char *http_date, time_t *);

int writelog(int, struct logging*);
void init_logging(struct logging*);

#endif // CONST_H_INCLUDED
