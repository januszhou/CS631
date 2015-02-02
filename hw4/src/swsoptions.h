#ifndef SWS_OPTIONS_H_INCLUDED
#define SWS_OPTIONS_H_INCLUDED

#include <sys/param.h>

#include <netinet/in.h>
#ifndef __sun__
#include <magic.h>
#endif
struct swsoptions {

    int f_debugging;
    int f_cgi;
    int f_bindadd;
    int f_logging;
    int f_port;
    char cgidir[MAXPATHLEN+1];
    char bindadd[INET6_ADDRSTRLEN];
    char logfile[MAXPATHLEN+1];
    char docroot[MAXPATHLEN+1];
    char *portno;
    #ifndef __sun__
    magic_t magic_cookie;
    #endif
    int logfd;
};

#endif // SWS_OPTIONS_H_INCLUDED
