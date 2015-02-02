#include <stdio.h>
#ifndef __sun__
#include <magic.h>
#endif
#include <string.h>
#include <errno.h>

#include "magic_utils.h"

int
initMagicLib(struct swsoptions *opt) {
#ifndef __sun__
    opt->magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (opt->magic_cookie == NULL) {
        fprintf(stderr,"initialize magic error: %s\n",strerror(errno));
        return 1;
    }

    if (magic_load(opt->magic_cookie, NULL) != 0) {
        fprintf(stderr,"cannot load magic database : %s\n", magic_error(opt->magic_cookie));
        magic_close(opt->magic_cookie);
        return 1;
    }
#endif
    return 0;
  
}

const char *
getMIMEForFile(struct swsoptions *opt,char * filename) {
    #ifndef __sun__
      return  magic_file(opt->magic_cookie, filename);
    #else
      return "text/html";
    #endif

}
