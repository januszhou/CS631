#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "swsoptions.h"

void initServer(struct swsoptions *);
void acceptConnection(struct swsoptions *);
void handleRequest(struct swsoptions *, int);

#endif // NET_H_INCLUDED
