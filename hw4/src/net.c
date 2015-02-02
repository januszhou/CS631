#include <sys/types.h>
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

#include "const.h"
#include "http.h"
#include "magic_utils.h"
#include "net.h"
#include "util.h"

int sock;

/* initServer() maps the host to a given IP address and port number using getaddrinfo(), creates a socket of internet 
 * domain, binds given address with a socket and finally server runs as a daemon and starts listening for incoming requests.
 * The number of outstanding requests that kernel will queue for host is 20. 
 */

void
initServer(struct swsoptions *opt) {

  if(!opt->f_port) {

    if((opt->portno = (char*)malloc(strlen(DEFAULT_PORT) + 1)) == NULL) {
      fprintf(stderr,"malloc error: %s\n",strerror(errno));
      exit(EXIT_FAILURE);
    }

    strncpy(opt->portno, DEFAULT_PORT,strlen(DEFAULT_PORT));
    opt->portno[strlen(DEFAULT_PORT)] = '\0';

  }

  struct addrinfo *ailist, *aip;
  struct addrinfo hint;

  int error, on = 1;

  hint.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = 0;
  hint.ai_addrlen = 0;
  hint.ai_canonname = NULL;
  hint.ai_addr = NULL;
  hint.ai_next = NULL;

  if(opt->f_bindadd) {

    if ((error = getaddrinfo(opt->bindadd, opt->portno , &hint, &ailist)) != 0) {
      fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(error));
      exit(EXIT_FAILURE);
    }

  } else {

    if ((error = getaddrinfo(NULL, opt->portno , &hint, &ailist)) != 0) {
      fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(error));
      exit(EXIT_FAILURE);
    }
  }

  pid_t pid ;

  for (aip = ailist; aip != NULL; aip = aip->ai_next) {

    sock = socket(aip->ai_family, aip->ai_socktype, 0);
    if (sock < 0) {
      fprintf(stderr,"Error in opening stream socket\n");
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on,sizeof(on)) == -1)
      fprintf(stderr, "setsockopt SO_REUSEADDR: %s",strerror(errno));

    if (bind(sock, aip->ai_addr, aip->ai_addrlen) == 0) {

      pid = fork();

      if(pid>0){

        listen(sock, BACKLOG);
        break;
      }else if (pid==0){

        continue;
      }
      else
        perror("Error forking per socket\n");
    }
  }

  if (aip == NULL) {
    if(pid > 0) /*last child is supposed to exit here.*/
      fprintf(stderr, "Could not bind\n");
    exit(EXIT_FAILURE);
  }

  if(initMagicLib(opt)) {
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(ailist);

  if (!opt->f_debugging) {
    if (daemon(1, 1) < 0) {
      fprintf(stderr, "Cannot transit into daemon mode\n");
      exit(EXIT_FAILURE);
    }
  }
}

/* acceptConnection() runs a forever loop to accept incoming requests and fork a new process for every incoming
 * request. 
 */

void
acceptConnection(struct swsoptions *opt) {

  struct sockaddr_storage *client;
  socklen_t length;
  int msgsock, pid;

  do {
    length = sizeof(client);
    msgsock = accept(sock, (struct sockaddr *)&client, &length);
    if (msgsock == -1)
      perror("accept");
    else

      if((pid = fork()) < 0)
        perror("can not fork");
      else if(pid == 0) {
        handleRequest(opt,msgsock);
        exit(EXIT_SUCCESS);
      }

  } while (TRUE);

}
