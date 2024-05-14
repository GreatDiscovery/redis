#ifndef __TOPKEY_H
#define __TOPKEY_H

#include "server.h"

#define TYPE_HOT 5
#define TYPE_BIG_KEY 1<<0
#define TYPE_HOT_KEY 1<<1

#define topkey_push_entry_hotkey(key, value, count) { \
     PushEntry(key, value, count, TYPE_HOT);                                                 \
}

#define swap_entry(heap, x, y)  { \
    heap[x]->index=y;                   \
    heap[y]->index=x;                   \
    topEntry* t = heap[x];              \
    heap[x]=heap[y];                    \
    heap[y]=t;                          \
}

typedef struct topEntry {
    int type;
    sds key;
    long long count;
    int index;
    time_t timestamp;
} topEntry;

void topkeyInit(void);
void PushEntry(sds key, robj *value, long long count, int type);

#endif
