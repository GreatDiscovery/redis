
#include "topkey.h"

static topEntry **tophotkeys;
static int bigkey_capacity[6] = {0};

void topkeylogInit(void) {
    tophotkeys = zmalloc(sizeof(topEntry*) * 3);
    for (int i = 0; i < 3; ++i) {
        tophotkeys[i] = zmalloc(sizeof(topEntry));
    }
}

void topkeyUpdateEntry(sds key, robj *value, long long description, int type) {
    topEntry **topbigkeys;
    topbigkeys = tophotkeys;

}
