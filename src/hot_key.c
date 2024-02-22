//
// Created by 宋博文 on 2024/2/3.
//
#include "server.h"
#include "atomicvar.h"


static struct hotPoolEntry *HotPoolLFU;

void hotPoolAlloc(void) {
    struct hotPoolEntry *hp;
    int j;

    hp = zmalloc(sizeof(*hp) * HOTOOL_SIZE);
    for (j = 0; j < HOTOOL_SIZE; j++) {
        hp[j].counter = 0;
        hp[j].key = NULL;
        hp[j].cached = sdsnewlen(NULL, HOTOOL_CACHED_SDS_SIZE);
        hp[j].dbid = 0;
    }
    HotPoolLFU = hp;
}

void insertPool(int dbid, dictEntry *de, uint8_t counter) {
    int k = 0;
    sds key;
    key = dictGetKey(de);
    struct hotPoolEntry *pool = HotPoolLFU;
    while (k < HOTOOL_SIZE && pool[k].key && pool[k].counter < counter) {
        k++;
    }
    if (k == 0 && pool[HOTOOL_SIZE - 1].key != NULL) {
        /* Can't insert if the element is < the worst element we have
         * and there are no empty buckets. */
        return;
    } else if (k < HOTOOL_SIZE && pool[k].key == NULL) {
        /* Inserting into empty position. No setup needed before insert. */
    } else {
        /* Inserting in the middle. Now k points to the first element
         * less than the element to insert.  */
        if (pool[HOTOOL_SIZE - 1].key == NULL) {
            sds cached = pool[HOTOOL_SIZE - 1].cached;
            memmove(pool + k + 1, pool + k, sizeof(pool[0]) * (HOTOOL_SIZE - k - 1));
            pool[k].cached = cached;
        } else {
            k--;
            /* Shift all elements on the left of k (included) to the
             * left, so we discard the element with smaller counter. */
            sds cached = pool[0].cached;
            if (pool[0].key != pool[0].cached) {
                sdsfree(pool[0].key);
            }
            memmove(pool, pool + 1, sizeof(pool[0]) * k);
            pool[k].cached = cached;
        }
    }

    /* Try to reuse the cached SDS string allocated in the pool entry,
     * because allocating and deallocating this object is costly
     * (according to the profiler, not my fantasy. Remember:
     * premature optimization bla bla bla. */
    int klen = sdslen(key);
    if (klen > HOTOOL_CACHED_SDS_SIZE) {
        pool[k].key = sdsdup(key);
    } else {
        memcpy(pool[k].cached, key, klen + 1);
        sdssetlen(pool[k].cached, klen);
        pool[k].key = pool[k].cached;
    }
    pool[k].counter = counter;
    pool[k].dbid = dbid;
}
