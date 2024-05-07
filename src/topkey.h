#ifndef __TOPKEY_H
#define __TOPKEY_H

#include "server.h"

#define TYPE_HOT 5
#define TYPE_BIG_KEY 1<<0
#define TYPE_HOT_KEY 1<<1

typedef struct topEntry {
    int type;
    sds key;
    long long count;
    int index;
    time_t timestamp;
} topEntry;

void topkeyInit(void);
void PushEntry(sds key, robj *value, long long description, int type);

#endif
