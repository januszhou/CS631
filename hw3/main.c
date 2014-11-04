#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFSIZE (4 * 1024)
#define MIN_PORT 1
#define MAX_PORT 65535

int flag_c;
int flag_d;
int flag_i;
int flag_p;

char* dir;
char* address;
char*file;
int port;

int sock;
int msgsock;
int rval;
struct sockaddr_in server;
struct sockaddr_in client;
struct timeval to;
char buf[1024];
char *client_addr;
fd_set ready;
socklen_t length;

void
usage(){
  fprintf(stderr,"usage: sws [ −dh] [ −c dir] [ −i address] [ −l file] [ −p port] dir\n");
  err(EXIT_FAILURE, NULL);
}

int
is_dir(const char *dir)
{
  struct stat sb;

  if (dir == NULL) {
    warnx("the provided dir is NULL");
    return 0;
  }
  if (stat(dir, &sb) < 0) {
    warn("cannot stat dir %s", dir);
    return 0;
  }
  if (!S_ISDIR(sb.st_mode)) {
    warnx("path %s you provided is not a directory", dir);
    return 0;
  }

  return 1;
}

void 
net(){
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("opening stream socket");
    exit(1);
  }
  printf("socket success!,sockfd = %d\n",sock);

  server.sin_family = AF_INET;

  if (flag_i == 1)
  {
    server.sin_addr.s_addr = inet_addr(address);
  }else{
    server.sin_addr.s_addr = INADDR_ANY;
  }

  if (flag_p == 1)
  {
    server.sin_port = htons(port);
  }else{
    server.sin_port = 8080; 
  }

  if (bind(sock, (struct sockaddr *)&server, sizeof(server))) {
    perror("binding stream socket");
    exit(1);
  }

  printf("bind success!\n");

  length = sizeof(server);
  if (getsockname(sock, (struct sockaddr *)&server, &length)) {
    perror("Socket name: ");
    exit(1);
  }

  printf("Socket has port #%d\n", ntohs(server.sin_port));

  listen(sock, 5);
  do {
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    to.tv_sec = 5;
    to.tv_usec = 0;
    if (select(sock + 1, &ready, 0, 0, &to) < 0) {
      perror("select");
      continue;
    }
    if (FD_ISSET(sock, &ready)) {
      length = sizeof(client);
      msgsock = accept(sock, (struct sockaddr *)&client, &length);
      client_addr = inet_ntoa(client.sin_addr);
      printf("Client connection from %s!\n", client_addr);
      if (msgsock == -1)
        perror("accept");
      else do {
        bzero(buf, sizeof(buf));
        if ((rval = read(msgsock, buf, BUFSIZE)) < 0)
          perror("reading stream message");
        else if (rval == 0)
          printf("Ending connection from %s.\n", client_addr);
        else
          printf("Client (%s) sent: %s", client_addr, buf);
      } while (rval > 0);
      close(msgsock);
    }
  } while (1);
}

int 
main(int argc, char *argv[])
{
  char ch;

    while ((ch = getopt(argc, argv, "−c:dhi:l:p:"))!= -1) {
        switch (ch) {
            case 'c':
                dir = optarg;
                if (!is_dir(dir)) {
                  errx(EXIT_FAILURE, "invalid dir");
                }
                break;
            case 'd':
                flag_d = 1;
                break;
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            case 'i':
              flag_i = 1;
                address = optarg;
                break;
            case 'p':
              flag_p = 1;
              port = atoi(optarg);
              if ((port < MIN_PORT) || (port > MAX_PORT)) {
                errx(EXIT_FAILURE, "port must be between %d and %d", MIN_PORT,
                MAX_PORT);
              }
              break;
            default:
              usage();
              exit(EXIT_FAILURE);
        }
    }
    argc -=optind;
    argv +=optind;

    if (argc < 0)
    {
      usage();
    }

    net();
    
    return 0;
}