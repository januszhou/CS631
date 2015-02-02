#ifndef CGI_H_INCLUDED
#define CGI_H_INCLUDED
#include <stdbool.h>

#include "http.h"
#include "swsoptions.h"

void handle_cgi(struct swsoptions *,int, char *, struct http_req * ,char * , struct logging*);

#endif // CGI_H_INCLUDED
