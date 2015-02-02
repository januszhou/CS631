#include <sys/param.h>

#include <netinet/in.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "net.h"
#include "swsoptions.h"

void usage(char *);

/* This program implements the simple web server where it parses all command-line options, initialize the server, 
 * binds to given IP address and port number and runs forever to accept the incoming requests. If IP address and 
 * port are not given as command-line arguments then server will listen on all IPv4 and IPv6 addresses on host and 
 * default port will be 8080. All the requests are served relative to the given document root directory.
 */

int
main(int argc, char **argv) {

  struct swsoptions swsopt = {0};
  int ch;

  while((ch = getopt(argc,argv,"c:dhi:l:p:"))!=-1) {
    switch (ch) {
      case 'c':
        swsopt.f_cgi = 1;
        bzero(swsopt.cgidir, sizeof(swsopt.cgidir));
        realpath(optarg,swsopt.cgidir);
        break;

      case 'd':
        swsopt.f_debugging = 1;
        break;

      case 'h':
        usage(argv[0]);
        exit(EXIT_SUCCESS);
        break;

      case 'i':
        swsopt.f_bindadd= 1;
        bzero(swsopt.bindadd, sizeof(swsopt.bindadd));
        strncpy(swsopt.bindadd, optarg, strlen(optarg));
        break;

      case 'l':
        swsopt.f_logging = 1;
        bzero(swsopt.logfile, MAXPATHLEN+1);
        strncpy(swsopt.logfile, optarg, strlen(optarg));
        if((swsopt.logfd=open(swsopt.logfile,O_CREAT | O_APPEND | O_WRONLY,0666))<0){
          perror("Logfile error");
          exit(EXIT_FAILURE);
        }
        break;

      case 'p':
        swsopt.f_port = 1;

        if((swsopt.portno = (char*)malloc(strlen(optarg) + 1)) == NULL) {
          fprintf(stderr,"malloc error: %s\n",strerror(errno));
          exit(EXIT_FAILURE);

        }

        strncpy(swsopt.portno, optarg,strlen(optarg));
        swsopt.portno[strlen(optarg)] = '\0';

        break;

      case '?':
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  /* Check if document root directory is missing from the command line argument */
  if(argc<=optind) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  } else {

    DIR *dp;
    realpath(argv[argc-1],swsopt.docroot);

    if ((dp = opendir(swsopt.docroot)) == NULL ) {
      fprintf(stderr, "can't open '%s': %s\n", swsopt.docroot, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  initServer(&swsopt);
  acceptConnection(&swsopt);

  exit(EXIT_SUCCESS);
}


void usage(char * prog_name) {

  fprintf(stderr,"usage %s [-dh] -c[dir] -i[address] -l[file] -p[port] dir\n",prog_name);

}
