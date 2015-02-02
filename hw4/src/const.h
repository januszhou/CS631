#ifndef CONST_H_INCLUDED
#define CONST_H_INCLUDED

#define SERVER_HEAD_FIELD "Server: sws/1.0\r\n"
#define SERVER_NAME "sws/1.0"
#define DEFAULT_PORT "8080"
#define BACKLOG 20
#define TRUE 1
#define BUFSIZE 409600
#define STRBUF 4096
#define HTTP_ONEZ "HTTP/1.0"
#define HEADER_DATE_FORMAT "AAA, DD MMM YYYY HH:MM:SS TZT "
#define DATE_FORMAT "YYYY-MM-DD HH:MM:SS "

#define RES_STATUS_200 "200"
#define RES_STATUS_204 "204"
#define RES_STATUS_304 "304"
#define RES_STATUS_400 "400"
#define RES_STATUS_403 "403"
#define RES_STATUS_404 "404"
#define RES_STATUS_408 "408"
#define RES_STATUS_413 "413"
#define RES_STATUS_500 "500"
#define RES_STATUS_501 "501"
#define RES_STATUS_505 "505"

#define OK_200 "HTTP/1.0 200 OK\r\n"
#define NOTFOUND_404 "HTTP/1.0 404 Not Found\r\n"
#define BADREQ_400 "HTTP/1.0 400 Bad Request\r\n"
#define NOTIMP_501 "HTTP/1.0 501 Not Implemented\r\n"
#define NOCONT_204 "HTTP/1.0 204 No Content\r\n"
#define INTERNAL_SER_ERR_500 "HTTP/1.0 500 Internal Server Error\r\n"
#define PROTO_NOTOK_505 "HTTP/1.0 505 Version Not Supported\r\n"
#define REQ_TIME_OUT_408 "HTTP/1.0 408 Request Timeout\r\n"
#define REQ_ENTITY_TOO_LAR_413 "HTTP/1.0 413 Request Entity Too Large\r\n"
#define NOTMOD_304 "HTTP/1.0 304 Not Modified\r\n"
#define FORBIDDEN_403 "HTTP/1.0 403 Forbidden\r\n"

#define NOTFOUND_404_MSG "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>"

#define NOTIMP_501_MSG "<html><head><title>501 Not Implemented</title></head><body><h1>Not Implemented</h1><p>The requested HTTP method was not implemented on this server.</p></body></html>"

#define BADREQ_400_MSG "<html><head><title>400 Bad Request</title></head><body><h1>Bad Request</h1><p>Request is not valid.</p></body></html>"

#define INTERNAL_SER_ERR_500_MSG "<html><head><title>500 Internal Server Error</title></head><body><h1>Internal Server Error</h1><p>Error occured in performing the request</p></body></html>"

#define PROTO_NOTOK_505_MSG "<html><head><title>505 Version Not Supported</title></head><body><h1>Version Not Supported</h1><p>HTTP version not supported</p></body></html>"

#define REQ_TIME_OUT_408_MSG "<html><head><title>408 Request Timeout</title></head><body><h1>Request Timeout</h1><p>The server timed out waiting for the request</p></body></html>"

#define REQ_ENTITY_TOO_LAR_413_MSG "<html><head><title>413 Request Entity Too Large</title></head><body><h1>Request Entity Too Large</h1><p>The request is larger than the server is able to process</p></body></html>"

#define NOTMOD_304_MSG "<html><head><title>304 Not Modified</title></head><body><h1>Not Modified</h1><p>File has not been modified since the version specified by the request headers If-Modified-Since.</p></body></html>"

#define INDEX_HTML_STATICPART1 "<html><head><title>Index</title></head><body>"

#define INDEX_HTML_STATICPART2 "<table><tr><th>Name</th><th>Last Modified</th><th>Size</th></tr><tr><th colspan='10'><hr></th></tr>"

#define INDEX_HTML_STATICPART3 "<tr><th colspan='10'><hr></th></tr></table><address>sws/1.0</address></body></html>"

#define CONTENT_TYP "Content-Type:text/html\r\n"


#endif // CONST_H_INCLUDED
