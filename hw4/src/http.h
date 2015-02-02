#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "util.h"
#include "swsoptions.h"

struct http_req {

  char* http_method;
  char* URI;
  char* protocol;
  char* cgi_uri;
  char* if_modified_since;
  bool fhead_req;

};

void handleRequest(struct swsoptions *,int);
void list_dir_entry(struct swsoptions *, struct http_req *, int, char *, struct logging *);
void send_file(struct swsoptions *, struct http_req *, int, char *, struct logging *);
void sendOKHeader(struct swsoptions *, int, char *, char *, struct logging *);
void sendHeader(struct swsoptions *, int, char *, char *, struct logging *);
int parse_request_header(struct http_req *, char *);
void serve_request(struct swsoptions *, struct http_req *, int, struct logging *);
int is_file_within_docroot(char *, char *, char *) ;
int validate_req(struct http_req *);
void request_header(char *buf);
#endif // HTTP_H_INCLUDED
