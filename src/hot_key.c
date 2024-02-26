//
// Created by 宋博文 on 2024/2/3.
//
#include "server.h"
#include "atomicvar.h"


static struct hotPoolEntry *HotPoolLFU;

// TODO: optimise time complexity
void hotkeyCron(void) {
    serverLog(LL_NOTICE, "entering hotkey Cron");
    struct hotPoolEntry *pool = HotPoolLFU;
    for (int i = 0; i < HOTOOL_SIZE; ++i) {
        if (pool[i].key) {
            robj* rk = createRawStringObject(pool[i].key, sizeof(pool[i].key));
            // fixme: what if key is expired
            lookupKeyReadWithFlags(0, rk, LOOKUP_NONE);
        }
    }
}

void hotPoolAlloc(void) {
    struct hotPoolEntry *hp;
    int j;

    hp = zmalloc(sizeof(*hp) * HOTOOL_SIZE);
    for (j = 0; j < HOTOOL_SIZE; j++) {
        hp[j].counter = 0;
        hp[j].key = NULL;
    }
    HotPoolLFU = hp;
}

void hotkeyCommand(client *c) {
    sds hot_keys = sdsempty();
    for (int i = 0; i < HOTOOL_SIZE; i++) {
        if (HotPoolLFU[i].key != NULL) {
            hot_keys = sdscatprintf(hot_keys, "key:%s, count:%llu;", HotPoolLFU[i].key, HotPoolLFU[i].counter);
        }
    }
    addReplyBulkSds(c, hot_keys);
}


void insertPool(dictEntry *de, uint8_t counter) {
    int i = 0, k = 0;
    sds key;
    key = dictGetKey(de);
    struct hotPoolEntry *pool = HotPoolLFU;
    // TODO: use condition to avoid entering this branch
    while (i < HOTOOL_SIZE) {
        if (pool[i].key && !sdscmp(key, pool[i].key)) {
            sdsfree(pool[i].key);
            pool[i].key = 0;
            memmove(pool + i, pool + i + 1, sizeof(pool[0]) * (HOTOOL_SIZE - i));
            if (pool[HOTOOL_SIZE-1].key) {
                pool[HOTOOL_SIZE-1].key = NULL;
                // fixme why?
//                sdsfree(pool[HOTOOL_SIZE - 1].key);
                pool[HOTOOL_SIZE - 1].counter = 0;
            }
            break;
        }
        i++;
    }
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
            memmove(pool + k + 1, pool + k, sizeof(pool[0]) * (HOTOOL_SIZE - k - 1));
        } else {
            k--;
            /* Shift all elements on the left of k (included) to the
             * left, so we discard the element with smaller counter. */
            sdsfree(pool[0].key);
            memmove(pool, pool + 1, sizeof(pool[0]) * k);
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
        pool[k].key = sdsdup(key);
    }
    pool[k].counter = counter;
}
