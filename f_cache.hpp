#ifndef __F_CACHE_H__
#define __F_CACHE_H__

#include <pthread.h>
#include <stack>
#include <stdint.h>
#include "hash_bucket.hpp"


class FCKeyIf
{
public:
    virtual uint32_t getHashVal() = 0;
};


template<class FCVal>
class FCache
{
public:
    FCache() {}
    ~FCache() {}
    int init() {}
    int start() {}
    int stop() {}
    FCNode<FCVal>* getNode(const FCKeyIf &key);
    FCNode<FCVal>* insertNode(const FCKeyIf &key) {}

private:
    HashBucket<FCVal> *m_hashMap;
    std::stack<FCNode<FCVal>*> m_idleNodes;
    pthread_mutex_t m_idleNodesLock;
};


template<class FCVal>
FCNode<FCVal>*
FCache<FCVal>::getNode(const FCKeyIf &key) {
    return NULL;
}


#endif
