#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <signal.h>
#include <libgen.h>
#ifdef __sun__
#include <strings.h>
#endif

#include "const.h"
#include "cgi.h"
#include "http.h"
#include "util.h"

/* handle_cgi() sets the environment variables for executing the given file, opens a pipe and fork a new process.
 * The child process will make one end of the pipe be the copy of stdout and call execve() for given file. The parent 
 * process will read from other end of the pipe and sends the output on connecting socket.   
 */

void
handle_cgi(struct swsoptions *opt,int msgsock, char *filepath, struct http_req *hr,char * query_string, struct logging* log) {

  if(!query_string)
    query_string="";
  char buf[BUFSIZE];

  bzero(buf, BUFSIZE);
  char   *cmd_args[] = { basename(filepath),
    NULL
  };
  char query_env[strlen("QUERY_STRING=")+strlen(query_string)] ;
  query_env[0]='\0';

  strncat(query_env,"QUERY_STRING=",strlen("QUERY_STRING="));

  if(query_string)
    strncat(query_env,query_string,strlen(query_string));

  /* should we support post ? */
  char * req_method ="REQUEST_METHOD=GET";
  if(hr->fhead_req){
    req_method ="REQUEST_METHOD=HEAD";
  }

  char * server_name = "SERVER_NAME=sws/1.0";

  int pro_len = strlen("SERVER_PROTOCOL=") + strlen(HTTP_ONEZ);
  char server_protocol[pro_len + 1];
  bzero(server_protocol,sizeof(server_protocol));
  strncpy(server_protocol,"SERVER_PROTOCOL=",strlen("SERVER_PROTOCOL="));
  strncat(server_protocol,HTTP_ONEZ,strlen(HTTP_ONEZ));

  int port_len = strlen("SERVER_PORT=") + strlen(opt->portno);
  char server_port[port_len + 1];
  bzero(server_port,sizeof(server_port));
  strncpy(server_port,"SERVER_PORT=",strlen("SERVER_PORT="));
  strncat(server_port,opt->portno,strlen(opt->portno));

  int pwd_len = strlen("PWD=") + strlen(opt->cgidir);
  char pwd[pwd_len + 1];
  bzero(pwd,sizeof(pwd));
  strncpy(pwd,"PWD=",strlen("PWD="));
  strncat(pwd,opt->cgidir,strlen(opt->cgidir));

  int uri_len = strlen("REQUEST_URI=") + strlen(hr->URI);
  char req_uri[uri_len + 1];
  bzero(req_uri,sizeof(req_uri));
  strncpy(req_uri,"REQUEST_URI=",strlen("REQUEST_URI="));
  strncat(req_uri,hr->URI,strlen(hr->URI));

  char *envp[] = {
    req_method ,
    query_env,
    server_name,
    server_protocol,
    server_port,
    pwd,
    req_uri,
    0
  };  

  int p[2];

  if(pipe(p) < 0) {
    sendHeader(opt,msgsock, INTERNAL_SER_ERR_500,INTERNAL_SER_ERR_500_MSG, log);
    if(!hr->fhead_req)
      send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
    return;

  }

  int status;

  int pid = fork();

  if (pid == -1) {

    sendHeader(opt,msgsock, INTERNAL_SER_ERR_500,INTERNAL_SER_ERR_500_MSG, log);
    if(!hr->fhead_req)
      send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
    return;
  }

  if (pid == 0) {

    if(dup2(p[1], STDOUT_FILENO) == -1) {

      sendHeader(opt,msgsock, INTERNAL_SER_ERR_500,INTERNAL_SER_ERR_500_MSG, log);
      if(!hr->fhead_req)
        send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
      return;
    }

    close(p[0]);

    if( execve(filepath, cmd_args, envp) == -1) {
      if(opt->f_debugging){
        perror("execve");
      }
      sendHeader(opt,msgsock, INTERNAL_SER_ERR_500,INTERNAL_SER_ERR_500_MSG, log);
      if(!hr->fhead_req)
        send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
      return;
    }

  } else {

    close(p[1]);

    char buf[BUFSIZE];
    int n;
    bzero(buf,BUFSIZE);
    char output_buf[BUFSIZE];
    char ftime[sizeof(HEADER_DATE_FORMAT)];

    getCurrentTime(ftime, sizeof(ftime));

    send(msgsock, OK_200, strlen(OK_200), 0);
    bzero(output_buf, BUFSIZE);

    strncpy(output_buf, "Date: ", strlen("Date: "));
    strncat(output_buf,ftime, strlen(ftime));
    strncat(output_buf,"\r\n", strlen("\r\n"));
    send(msgsock, output_buf,strlen(output_buf),0);
    send(msgsock, SERVER_HEAD_FIELD,strlen(SERVER_HEAD_FIELD),0);

    while ((n=read(p[0], buf, BUFSIZE))>0){

      if(hr->fhead_req) {
        char *  header_end=NULL;
        char * foundstr = "\r\n\r\n";
        if((header_end=strstr(buf,"\r\n\r\n") ) ==NULL){
          header_end=strstr(buf,"\n\n");
          foundstr= "\n\n";
        }
        if(header_end!= NULL) {
          *(header_end+strlen(foundstr))='\0';
          send(msgsock, buf, strlen(buf), 0);
        }
      }else{
        send(msgsock, buf, n, 0);
      }
    }
    close(p[0]);
    waitpid(pid, &status, 0);

  }

}
