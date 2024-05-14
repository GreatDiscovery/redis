
#include "topkey.h"

dict *hotkey_whitelist = NULL;
static topEntry **tophotkeys;
// 记录每种类型的top数量
static int topkey_capacity[6] = {0};

static dictType whitelistDictType = {
        dictSdsHash,               /* hash function */
        NULL,                      /* key dup */
        NULL,                      /* val dup */
        dictSdsKeyCompare,         /* key compare */
        dictSdsDestructor,                      /* key destructor */
        NULL,                      /* val destructor */
        NULL                       /* allow to expand */
};

void topkeyInit(void) {
    tophotkeys = zmalloc(sizeof(topEntry *) * 3);
    for (int i = 0; i < 3; ++i) {
        tophotkeys[i] = zmalloc(sizeof(topEntry));
    }
    hotkey_whitelist = dictCreate(&whitelistDictType);
}

// 获取父节点索引
int getParentIndex(int childIndex) {
    return (childIndex - 1) / 2;
}

// 获取左子节点索引
int getLeftChildIndex(int parentIndex) {
    return 2 * parentIndex + 1;
}

// 获取右子节点索引
int getRightChildIndex(int parentIndex) {
    return 2 * parentIndex + 2;
}

void adjustUp(int index, topEntry **heap) {
    int parent_index = getParentIndex(index);
    while (index > 0 && heap[index] < heap[parent_index]) {
        swap_entry(heap, index, parent_index);
        index = parent_index;
        parent_index = getParentIndex(parent_index);
    }
}

void adjustDown(int index, int heapSize, topEntry **heap) {
    int minIndex = index;
    int leftChildIndex = getLeftChildIndex(index);
    int rightChildIndex = getRightChildIndex(index);
    // 找到当前节点、左子节点和右子节点中的最小值
    if (leftChildIndex < heapSize && heap[leftChildIndex] < heap[minIndex]) {
        minIndex = leftChildIndex;
    }
    if (rightChildIndex < heapSize && heap[rightChildIndex] < heap[minIndex]) {
        minIndex = rightChildIndex;
    }
    // 如果最小值不是当前节点，则交换它们的值并继续下移操作
    if (index != minIndex) {
        swap_entry(heap, index, minIndex);
        adjustDown(minIndex, heapSize, heap);
    }
}



void PushEntry(sds key, robj *value, long long count, int type) {
    dict *whitelist;
    topEntry **topkeys;
    int top_type = TYPE_HOT_KEY;
    switch (type) {
        case TYPE_HOT:
            whitelist = hotkey_whitelist;
            topkeys = tophotkeys;
            top_type = TYPE_HOT_KEY;
            break;
        case OBJ_STRING:
            break;
        default:
            break;
    }

    // 1. 先从白名单里找，如果在白名单里，寻找数组中的索引
    int index = -1;
    if (value->top_type != 0) {
        dictEntry *de = dictFind(whitelist, key);
        if (de) { // exist
            topEntry *te = dictGetVal(de);
            index = te->index;
        }
    }

    // 首次记录，直接插入
    if (topkey_capacity[type] == 0) {
        topkeys[0]->type = type;
        topkeys[0]->count = count;
        topkeys[0]->key = sdsdup(key);
        topkeys[0]->index = 0;
        topkeys[0]->timestamp = server.unixtime;
        topkey_capacity[type] += 1;
        value->top_type |= top_type;
        dictAdd(whitelist, topkeys[0]->key, topkeys[0]);
        return;
    }

    // key已经存在，更新key
    if (index != -1) {
        topEntry *te = topkeys[index];
        if (!te) {
            dictDelete(whitelist, key);
            topkey_capacity[type] = topkey_capacity[type] - 1 < 0 ? 0 : topkey_capacity[type] - 1;
            return;
        }
        te->count = count;
        int parent_index = getParentIndex(index);
        if (count > topkeys[parent_index]->count) {
            adjustUp(index, topkeys);
        } else {
            adjustDown(index, topkey_capacity[type], topkeys);
        }
        te->timestamp = server.unixtime;
        return;
    }
}

