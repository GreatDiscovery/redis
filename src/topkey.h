#ifndef __TOPKEY_H
#define __TOPKEY_H

#include "server.h"

typedef struct topEntry {
    sds key;
    long long description;
    int type;
    int index;
    time_t timestamp;
} topEntry;

void topkeylogInit(void);
void topkeyUpdateEntry(sds key, robj *value, long long description, int type);

#endif
