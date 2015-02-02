#ifndef MAGIC_UTILS_H_INCLUDED
#define MAGIC_UTILS_H_INCLUDED

#include "swsoptions.h"

int initMagicLib(struct swsoptions *);
const char * getMIMEForFile(struct swsoptions *, char *);

#endif // MAGIC_UTILS_H_INCLUDED
