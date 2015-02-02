#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pwd.h>
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
#ifdef __sun__
#include <strings.h>
#endif

#include "const.h"
#include "cgi.h"
#include "magic_utils.h"
#include "net.h"
#include "util.h"

/* minimum http request */
#define HTTP_REQ "GET / HTTP/1.0"

#ifndef REQ_WAIT_TIME
#define REQ_WAIT_TIME 60
#endif

#define MAX_URL_LEN MAXPATHLEN

volatile sig_atomic_t  is_alarm_expire;

/* parse_request_header() processes each incoming request and extracts requested method, URI, protocol and 
 * if-modified-since date from request header.
 */

int 
parse_request_header(struct http_req *hr,char *buf) {
  char *str1, *str2, *token;
  char *saveptr1, *saveptr2;
  int j;
  char *freqline = NULL, *subtoken[3], *if_mod_str = NULL;

  for (j = 1, str1 = buf; ; j++, str1 = NULL) {
    token = strtok_r(str1,"\r\n", &saveptr1);
    if (token == NULL)
      break;

    if(j == 1)
      freqline = token;

    if(strncasecmp(token,"if-modified-since",strlen("if-modified-since")) == 0) {

      str2 = token;
      strtok_r(str2,":", &saveptr2);

      while(isspace(*saveptr2 - '0')) 
        saveptr2++;

      if_mod_str = saveptr2;

    }

  }

  /* If the length of request's first line is less the the length of "GET / HTTP/1.0" string then request is 
   * invalid.
   */
  if(strlen(freqline) < strlen(HTTP_REQ)){
    return -1;
  }

  for ( j = 0, str2 = freqline; ; j++, str2 = NULL) {
    if((token = strtok_r(str2," ", &saveptr1)) == NULL)
      break;
    else {
      if(j<3)
        subtoken[j] = token;
    }
  }

  /* Format of request is Request-Line = Method SP Request-URI SP HTTP-Version CRLF. So if there are more than 3 
   * tokens seperated by SP in a request line its a invalid request. 
   */
  if(j>3)
    return -1;

  hr->http_method = subtoken[0];
  hr->URI = subtoken[1];
  hr->protocol = subtoken[2];
  hr->if_modified_since = if_mod_str;
  return 0;
}

/* SIGALRM signal handler function */
static void
sig_alarm(int signo) {

  is_alarm_expire++;
}

/* handleRequest() reads the data from socket and uses getnameinfo() to get the client ip address. 
 * If the connection remains ideal for 60 sec, it sends HTTP 408 response. Otherwise validates the incoming 
 * requests by calling validate_req(). If request is valid and supported, it calls serve_request() to send 
 * approriate response of given request. After serving the request it closes the connection with the client.
 */

void
handleRequest(struct swsoptions *opt,int msgsock) {

  char buf[BUFSIZE];

  int rval;
  const char *client_add;
  char addbuf[NI_MAXHOST];
  struct sockaddr_storage *client;
  socklen_t len;
  struct logging log;

  bzero(addbuf, sizeof(addbuf));
  len = sizeof(struct sockaddr_storage);
  if ((client = malloc(len)) == NULL) {
    fprintf(stderr,"malloc error: %s\n",strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(getpeername(msgsock, (struct sockaddr *)client, &len) < 0) {

    client_add = NULL;

  } else {

    if(getnameinfo((struct sockaddr *)client, len, addbuf, sizeof(addbuf), NULL, 0, NI_NUMERICHOST) == 0) {
      client_add = addbuf;
    }
  }

  /* process log client ip */
  strncpy(log.remoteip, client_add, sizeof(log.remoteip) - 1);

  if(signal(SIGALRM,sig_alarm) == SIG_ERR){
    fprintf(stderr,"Unable to establish alarm signal handler: %s",strerror(errno));
  }
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sig_alarm;
  sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGALRM);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

  alarm(REQ_WAIT_TIME); /* time out after REQ_WAIT_TIME seconds */


  bzero(buf, sizeof(buf));
  rval = read(msgsock, buf, BUFSIZE);
  alarm(0);

  if ( rval<0 && errno == EINTR){  

    sendHeader(opt,msgsock, RES_STATUS_408,REQ_TIME_OUT_408_MSG, &log);
    send(msgsock, REQ_TIME_OUT_408_MSG, strlen(REQ_TIME_OUT_408_MSG), 0);
    shutdown (msgsock, SHUT_RDWR);
    close(msgsock);
    return;

  }else if(rval<0){
    perror("reading stream message");
    return;
  }
  else if (rval == 0){
    if(opt->f_debugging){
      printf("\nEnding connection with client : %s \n",client_add);
    } 
  }
  else {
    if(opt->f_debugging){
      printf("Client (%s) sent: %s", client_add, buf);  
    } 
  }

  struct http_req hr = {0};
  hr.fhead_req = false;

  /* save the log */
  strncpy(log.remoteip, client_add, sizeof(log.remoteip) - 1);  
  strncpy(log.request_lineq, buf, sizeof(log.request_lineq) - 1);

  if(parse_request_header(&hr, buf) < 0 || validate_req(&hr) < 0) {

    sendHeader(opt,msgsock, RES_STATUS_400,BADREQ_400_MSG, &log);

    send(msgsock, BADREQ_400_MSG, strlen(BADREQ_400_MSG), 0);

  }else {

    if(strncasecmp(hr.http_method,"GET",3) == 0 || strncasecmp(hr.http_method,"HEAD",4) == 0 ) {

      if(strncasecmp(hr.http_method,"HEAD",4) == 0)
        hr.fhead_req = true;

      if(strncasecmp(hr.protocol, "HTTP/1.0", strlen("HTTP/1.0")) != 0){

        sendHeader(opt,msgsock, RES_STATUS_505,PROTO_NOTOK_505_MSG, &log);

        if(!hr.fhead_req)
          send(msgsock, PROTO_NOTOK_505_MSG, strlen(PROTO_NOTOK_505_MSG), 0);
      }else {

        serve_request(opt, &hr, msgsock, &log);
      }

    }
    else {
      sendHeader(opt,msgsock, RES_STATUS_501,NOTIMP_501_MSG, &log);

      send(msgsock, NOTIMP_501_MSG, strlen(NOTIMP_501_MSG), 0);
    }
  }

  shutdown (msgsock, SHUT_RDWR);
  close(msgsock);
}

/* validate_req() makes sure that the given request is valid. If request is invalid it returns -1 otherwise 0. 
 * The request is invalid when http method or uri or protocol is missing in the request or http_method is a string 
 * other than any HTTP methods or url does not starts with / or protocol string does not start with "HTTP/". 
 */

int
validate_req(struct http_req *hr) {

  if(hr->http_method == NULL || hr->URI == NULL || hr->protocol == NULL){
    return -1;
  }

  if(strncasecmp(hr->http_method,"GET",3) == 0 || strncasecmp(hr->http_method,"HEAD",4) == 0 || 
     strncasecmp(hr->http_method,"POST",4) == 0 ||strncasecmp(hr->http_method,"PUT",3) == 0 || 
     strncasecmp(hr->http_method,"DELETE",6) == 0 || strncasecmp(hr->http_method,"CONNECT",7) == 0 || 
     strncasecmp(hr->http_method,"OPTIONS",7) == 0 || strncasecmp(hr->http_method,"TRACE",5) ==0 ) {

    if(strncmp(hr->URI,"/",1) != 0){
      return -1;
    }

    if(strncasecmp(hr->protocol, "HTTP/", 5) != 0){
      return -1;
    }    
    return 0;

  }else 
    return -1;

  return 0;

}

/* serve_request() either send a appropriate error message or send the requested file data. It sends HTTP 413 
 * error if requested URL is too large or sends HTTP 404 file not found if requested url is outside the
 * docroot directory. If requested file is within docroot it checks the file type, if its a directory it calls 
 * list_dir_entry() to list the directory content. In case of file it checks the if-modified-since date from request 
 * header is < the file last modified date, if so call send_file() to return the file data or return HTTP 304 respose 
 * status.
 */

void 
serve_request(struct swsoptions *opt, struct http_req *hr, int msgsock, struct logging* log) {

  char filepath[MAXPATHLEN+1];

  bzero(filepath, sizeof(filepath));

  /* If the full URl of requested file is greater than the MAX_URL_LEN, send HTTP 413 error.*/
  if(strlen(opt->docroot) + strlen(hr->URI) > MAX_URL_LEN){
    sendHeader(opt,msgsock, RES_STATUS_413,REQ_ENTITY_TOO_LAR_413_MSG, log);
    if(!hr->fhead_req)
      send(msgsock, REQ_ENTITY_TOO_LAR_413_MSG, strlen(REQ_ENTITY_TOO_LAR_413_MSG), 0);
    return;
  }

  URIDecode(hr->URI,hr->URI);

  char * str_for_cgi;
  bool is_cgi;
  is_cgi=false;
  char  * query_str;
  query_str=NULL;

  if(strncmp(hr->URI,"/~",2) == 0){
    struct passwd *pw;
    bzero(opt->docroot,sizeof(opt->docroot));
    char *str = hr->URI + 2; /* pointer points to the string starting from username. */

    int username_len;    
    const char* sptr = strchr(str,'/');

    if(sptr){
      username_len = strlen(str) - strlen(sptr);

    }else{ /* if there is no uri after usename */
      username_len = strlen(str);
    }
    char uname[username_len + 1];
    strncpy(uname,str,username_len);
    uname[username_len] = '\0';

    if((pw = getpwnam(uname)) == NULL){
      sendHeader(opt,msgsock, RES_STATUS_505,PROTO_NOTOK_505_MSG, log);
      if(!hr->fhead_req)
        send(msgsock, PROTO_NOTOK_505_MSG, strlen(PROTO_NOTOK_505_MSG), 0);
      return;
    }

    strncpy(opt->docroot,pw->pw_dir,strlen(pw->pw_dir));
    strncat(opt->docroot,"/sws",strlen("/sws"));

    char *suburi = hr->URI + username_len + 2;

    strncpy(filepath, opt->docroot, strlen(opt->docroot));

    if(suburi)
      strncat(filepath, suburi, strlen(suburi));

  }else if(strlen(opt->cgidir)>0 && strncmp(hr->URI,"/cgi-bin",strlen("/cgi-bin"))==0) {

    is_cgi = true;

    bzero(opt->docroot,sizeof(opt->docroot));
    strncpy(opt->docroot, opt->cgidir, strlen(opt->cgidir));
    strncpy(filepath, opt->docroot, strlen(opt->docroot));

    str_for_cgi = hr->URI + strlen("/cgi-bin");

    if(str_for_cgi) {
      strncat(filepath, str_for_cgi, strlen(str_for_cgi));
      query_str=strchr(filepath,'?');

      if(query_str) {

        query_str[0] = '\0';
        query_str+=1;

      }
    }
  }else {

    strncpy(filepath, opt->docroot, strlen(opt->docroot));
    if(strlen(hr->URI) > 1) {

      if(strncmp(hr->URI,"/",1) == 0)
        strncat(filepath, hr->URI, strlen(hr->URI));
      else {
        strncat(filepath, "/", 1);
        strncat(filepath, hr->URI, strlen(hr->URI));
      }
    }

  }

  char fileabspath[MAXPATHLEN+1];
  bzero(fileabspath,MAXPATHLEN+1);

  realpath(filepath,fileabspath);

  struct stat sb;
  time_t last_mod_tm;
  if (stat(fileabspath, &sb) == -1) {

    if(errno == ENOENT) {
      sendHeader(opt,msgsock, RES_STATUS_404,NOTFOUND_404_MSG, log);
      if(!hr->fhead_req)
        send(msgsock, NOTFOUND_404_MSG, strlen(NOTFOUND_404_MSG), 0);
    } else {
      sendHeader(opt,msgsock, RES_STATUS_500,INTERNAL_SER_ERR_500_MSG, log);

      if(!hr->fhead_req)
        send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
    }

  } else {

    if (S_ISDIR(sb.st_mode)) {

      if(is_file_within_docroot(opt->docroot,fileabspath, "DIR"))
        list_dir_entry(opt, hr, msgsock, fileabspath, log);
      else{
        sendHeader(opt,msgsock, RES_STATUS_404,NOTFOUND_404_MSG, log);

        if(!hr->fhead_req)
          send(msgsock, NOTFOUND_404_MSG, strlen(NOTFOUND_404_MSG), 0);
      }
    } else if (S_ISREG(sb.st_mode)) {

      if(is_file_within_docroot(opt->docroot,fileabspath, "FILE")){

        if (is_cgi) {

          handle_cgi(opt,msgsock, filepath, hr,query_str, log);
          return;
        } else {

          if(hr->if_modified_since && parseHttpDate(hr->if_modified_since, &last_mod_tm) && last_mod_tm >= sb.st_mtime) {

            sendHeader(opt,msgsock, RES_STATUS_304,NOTMOD_304_MSG, log);

            if(!hr->fhead_req)
              send(msgsock, NOTMOD_304_MSG, strlen(NOTMOD_304_MSG), 0);

          }else
            send_file(opt, hr, msgsock, fileabspath, log); 

        }
      }
      else{
        sendHeader(opt,msgsock, RES_STATUS_404,NOTFOUND_404_MSG, log);
        if(!hr->fhead_req)
          send(msgsock, NOTFOUND_404_MSG, strlen(NOTFOUND_404_MSG), 0);
      }
    }
  }
}

/* is_file_within_docroot() helps to strict the user from browsering into server file system outside the document 
 * root directory.
 */

int 
is_file_within_docroot(char *docroot, char *fileabpath, char *filetype) {

  if(strncmp(filetype,"DIR",3) == 0){
    return strlen(fileabpath) >= strlen(docroot); 
  }else {
    char copypath[strlen(fileabpath)+1];

    strncpy(copypath,fileabpath,strlen(fileabpath));

    copypath[strlen(fileabpath)+1] = '\0';

    char * parentdir = dirname(copypath);
    return strlen(parentdir) >= strlen(docroot);
  }

}

/* list_dir_entry() will call send_file() if index.html does exists in the requested directory otherwise it 
 * scans the requested directory using scandir() and sort them in alphanumeric order and generate a directory 
 * index for the directory.
 */

void 
list_dir_entry(struct swsoptions *opt, struct http_req *hr, int msgsock, char *abpath, struct logging* log) {

  struct dirent **namelist;
  int n, fd;
  int buflen = 2*BUFSIZE;
  char dirpath[strlen(abpath) + 1];
  char buf[buflen];
  bzero(buf, buflen);
  strncpy(dirpath, abpath, strlen(abpath));
  dirpath[strlen(abpath)] = '\0';

  strncat(abpath,"/index.html",strlen("/index.html"));

  if((fd=open(abpath, O_RDONLY)) != -1) {
    send_file(opt, hr, msgsock, abpath, log);
  } else {

    n = scandir(dirpath, &namelist, NULL, alphasort);

    if (n < 0)
      perror("scandir");
    else {
      /* This fpath contains the absolute path of files in a scandir directory.*/
      char fpath[MAXPATHLEN];
      bzero(fpath, MAXPATHLEN);
      char ftime[strlen(DATE_FORMAT)];


      URIDecode(hr->URI,hr->URI);
      strncpy(buf, INDEX_HTML_STATICPART1, strlen(INDEX_HTML_STATICPART1));
      strncat(buf, "<h1>Index of ", strlen("<h1>Index of "));
      strncat(buf,hr->URI , strlen(hr->URI));
      strncat(buf, "</h1>", strlen("</h1>"));

      strncat(buf, INDEX_HTML_STATICPART2, strlen(INDEX_HTML_STATICPART2));
      int i;
      struct stat sb;
      for (i = 0; i < n; i++) {
        if(strncmp(namelist[i]->d_name,".",1) == 0) {
          free(namelist[i]);
          continue;
        }
        bzero(fpath, MAXPATHLEN);
        strncpy(fpath,dirpath,strlen(dirpath));

        if(strncmp(fpath + (strlen(fpath) - 1), "/", 1) != 0)
          strncat(fpath, "/", 1);

        strncat(fpath, namelist[i]->d_name, strlen(namelist[i]->d_name));

        strncat(buf, "<tr><td style='padding:0 15px 0 15px;' ><a href=\"javascript:location=escape('", 
        strlen("<tr><td style='padding:0 15px 0 15px;' ><a href=\"javascript:location=escape('"));

        strncat(buf, hr->URI, strlen(hr->URI));

        if(strncmp(hr->URI + (strlen(hr->URI) - 1), "/", 1) != 0)
          strncat(buf, "/", 1);

        strncat(buf, namelist[i]->d_name, strlen(namelist[i]->d_name));
        strncat(buf, "')\">", strlen("')\">")) ;
        strncat(buf, namelist[i]->d_name , strlen(namelist[i]->d_name)) ;
        strncat(buf,"</a></td>", strlen("</a></td>"));

        if(stat(fpath, &sb) != -1 ) {
          time_t mtime;
          mtime = sb.st_mtime;
          char size_buf[3*sizeof(sb.st_size)];

          bzero(ftime, sizeof(ftime));
          bzero(size_buf,sizeof(size_buf));

          getFormattedTime(ftime, sizeof(ftime), mtime, DATE_FORMAT);

          strncat(buf, "<td style='padding:0 15px 0 15px;'>", strlen("<td style='padding:0 15px 0 15px;'>"));
          strncat(buf, ftime, strlen(ftime));
          strncat(buf,"</td>", strlen("</td>"));

          snprintf(size_buf, 3*sizeof(sb.st_size),"%lld",(long long int)sb.st_size);
          strncat(buf,"<td style='padding:0 15px 0 15px;'>", strlen("<td style='padding:0 15px 0 15px;'>"));
          strncat(buf, size_buf, strlen(size_buf));
          strncat(buf,"</td>", strlen("</td>"));

        }
        strncat(buf,"</tr>", strlen("</tr>"));
        free(namelist[i]);
      }
      strncat(buf, INDEX_HTML_STATICPART3, strlen(INDEX_HTML_STATICPART3));

      char size_as_str[STRBUF];
      snprintf ( size_as_str, sizeof(strlen(buf)), "%zu", strlen(buf));

      sendOKHeader(opt,msgsock, NULL,  size_as_str, log);
      if(!hr->fhead_req) 
        send(msgsock, buf,strlen(buf),0);

      free(namelist);
    }

  }

}

/* send_file() either sends OK response header followed by requested file data or HTTP 500 internal server error 
 * response if write function fails. 
 */

void
send_file(struct swsoptions *opt, struct http_req *hr, int msgsock, char *filepath, struct logging* log) {
  int fd, n;
  char buf[BUFSIZE];
  bzero(buf, BUFSIZE);

  if ( (fd=open(filepath, O_RDONLY)) != -1 ) {
    sendOKHeader(opt,msgsock, filepath,"", log);
    if(!hr->fhead_req) {
      while ((n = read(fd, buf ,BUFSIZE )) >0 )
        if (write (msgsock, buf, n) != n) {
          sendHeader(opt,msgsock, RES_STATUS_500,INTERNAL_SER_ERR_500_MSG, log);
          if(!hr->fhead_req)
            send(msgsock, INTERNAL_SER_ERR_500_MSG, strlen(INTERNAL_SER_ERR_500_MSG), 0);
        }
    }
  }
  else{
    sendHeader(opt,msgsock, RES_STATUS_403,NULL, log);
  }
  close(fd);
}

void
sendOKHeader(struct swsoptions *opt, int msgsock, char * file_path, char *content_len, struct logging* log) {

  char output_buf[BUFSIZE];
  char ftime[sizeof(HEADER_DATE_FORMAT)];
  struct stat fs;

  if(file_path) {
    if(stat(file_path, &fs) == -1 ) {
      fprintf(stderr,"stat error : %s\n",strerror(errno));
      return;
    }
  }
  getCurrentTime(ftime, sizeof(ftime));

  send(msgsock, OK_200, strlen(OK_200), 0);
  bzero(output_buf, BUFSIZE);

  strncpy(output_buf, "Date: ", strlen("Date: "));
  strncat(output_buf,ftime, strlen(ftime));
  strncat(output_buf,"\r\n", strlen("\r\n"));
  send(msgsock, output_buf,strlen(output_buf),0);

  send(msgsock, SERVER_HEAD_FIELD,strlen(SERVER_HEAD_FIELD),0);

  if(file_path!=NULL) {
    const char * content_type;
    send(msgsock, "Content-Type: ", strlen("Content-Type: "), 0);
    content_type = getMIMEForFile(opt,file_path);
    send(msgsock, content_type, strlen(content_type), 0);
    send(msgsock, "\r\n", strlen("\r\n"), 0);

    time_t mtime;
    mtime = fs.st_mtime;

    bzero(output_buf, BUFSIZE);
    bzero(ftime, sizeof(ftime));

    getFormattedTime(ftime, sizeof(ftime), mtime, HEADER_DATE_FORMAT);
    strncpy(output_buf, "Last-Modified: ", strlen("Last-Modified: "));
    strncat(output_buf, ftime, strlen(ftime));
    strncat(output_buf,"\r\n", strlen("\r\n"));
    send(msgsock, output_buf,strlen(output_buf),0);

  } else {
    send(msgsock, CONTENT_TYP, strlen(CONTENT_TYP), 0);
  }

  bzero(output_buf, BUFSIZE);
  strncpy(output_buf, "Content-Length: ", strlen("Content-Length: "));

  if(file_path!=NULL) {

    char size_buf[3*sizeof(fs.st_size)]; /* log10(2^(8*size)) = 2.4*size we use 3.0 as upper bound */

    snprintf(size_buf, 3*sizeof(fs.st_size),"%lld",(long long)fs.st_size);
    strncat(output_buf,size_buf, strlen(size_buf));
  } else {
    strncat(output_buf,content_len, strlen(content_len));
  }

  strncat(output_buf,"\r\n", strlen("\r\n"));
  send(msgsock, output_buf,strlen(output_buf),0);

  send(msgsock, "\r\n", strlen("\r\n"), 0);

  /* save log */
  strncpy(log->request_time, ftime, sizeof(log->request_time));
  strncpy(log->request_status, OK_200, sizeof(log->request_status) - 1);
  strncpy(log->response_size, content_len, sizeof(log->response_size));

  if (opt->f_debugging) {
    (void) writelog(STDOUT_FILENO, log);
  } else if (opt->f_logging) {
    (void) writelog(opt->logfd, log);
  }
}

void
sendHeader(struct swsoptions *opt, int msgsock, char *rstatus, char * content, struct logging* log) {
  
  char output_buf[BUFSIZE];
  char ftime[sizeof(HEADER_DATE_FORMAT)];
  getCurrentTime(ftime, sizeof(ftime));

  send(msgsock, "\r\n", strlen("\r\n"), 0);

  if(strncmp(rstatus,RES_STATUS_304, strlen(RES_STATUS_304)) == 0)
    send(msgsock, NOTMOD_304, strlen(NOTMOD_304),0);

  else if(strncmp(rstatus,RES_STATUS_413, strlen(RES_STATUS_413)) == 0)
    send(msgsock, REQ_ENTITY_TOO_LAR_413, strlen(REQ_ENTITY_TOO_LAR_413),0);

  else if(strncmp(rstatus,RES_STATUS_403, strlen(RES_STATUS_403)) == 0)
    send(msgsock, FORBIDDEN_403, strlen(FORBIDDEN_403),0);

  else if(strncmp(rstatus,RES_STATUS_501, strlen(RES_STATUS_501)) == 0)
    send(msgsock, NOTIMP_501, strlen(NOTIMP_501),0);

  else if(strncmp(rstatus,RES_STATUS_505, strlen(RES_STATUS_505)) == 0)
    send(msgsock, PROTO_NOTOK_505, strlen(PROTO_NOTOK_505),0);

  else if(strncmp(rstatus,RES_STATUS_400, strlen(RES_STATUS_400)) == 0)
    send(msgsock, BADREQ_400, strlen(BADREQ_400),0);

  else if(strncmp(rstatus,RES_STATUS_408, strlen(RES_STATUS_408)) == 0)
    send(msgsock, REQ_TIME_OUT_408, strlen(REQ_TIME_OUT_408),0);

  else if(strncmp(rstatus,RES_STATUS_404, strlen(RES_STATUS_404)) == 0)
    send(msgsock, NOTFOUND_404, strlen(NOTFOUND_404),0);

  else if(strncmp(rstatus,RES_STATUS_500, strlen(RES_STATUS_500)) == 0)
    send(msgsock, INTERNAL_SER_ERR_500, strlen(INTERNAL_SER_ERR_500),0);

  bzero(output_buf, BUFSIZE);

  strncpy(output_buf, "Date: ", strlen("Date: "));
  strncat(output_buf,ftime, strlen(ftime));
  strncat(output_buf,"\r\n", strlen("\r\n"));
  send(msgsock, output_buf,strlen(output_buf),0);

  send(msgsock, SERVER_HEAD_FIELD,strlen(SERVER_HEAD_FIELD),0);
  send(msgsock, CONTENT_TYP, strlen(CONTENT_TYP), 0);

  bzero(output_buf, STRBUF);
  char size_as_str[STRBUF];
  if(content!=NULL) {
    
    snprintf ( size_as_str, sizeof(strlen(content)), "%zu", strlen(content));
    strncpy(output_buf, "Content-Length: ", strlen("Content-Length: "));
    strncat(output_buf,size_as_str, strlen(size_as_str));
    
  }
  else{ 
    strncpy(output_buf, "Content-Length: 0", strlen("Content-Length: 0"));
  }
  strncat(output_buf,"\r\n", strlen("\r\n"));
  strncat(output_buf,"\r\n", strlen("\r\n"));
  send(msgsock, output_buf,strlen(output_buf),0);

  /* when response not ok, then we set response size is 0. */
  strncpy(log->request_time, ftime, sizeof(log->request_time));
  strncpy(log->request_status, rstatus, sizeof(log->request_status) - 1);
  if(content!=NULL)
    strncpy(log->response_size, size_as_str, strlen(size_as_str));
  else
    strncpy(log->response_size, "0", 1);
  
  /*
  write log based on sws options, if debugging, output to stdout, if logging, write to file.
   */
  if (opt->f_debugging) {
    (void) writelog(STDOUT_FILENO, log);
  } else if (opt->f_logging) {
    (void) writelog(opt->logfd, log);
  }
}
