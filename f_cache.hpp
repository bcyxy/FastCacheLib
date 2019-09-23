#ifndef __F_CACHE_H__
#define __F_CACHE_H__

#include <pthread.h>
#include <stack>
#include <stdint.h>
#include "hash_bucket.hpp"


/** Fast cache
 * 推荐用法：
 *     - 使用前预估元素数量，尽量大的设置hash数组长度，避免桶中有较多元素（桶的访问是加锁的）；*/
template<class FCVal>
class FCache
{
public:
    FCache(uint32_t hashSize, uint32_t nodeCount);
    ~FCache();
    int init() {}
    int start() {}
    int stop() {}
    FCNode<FCVal>* getNode(const FCKeyIf &key);
    FCNode<FCVal>* insertNode(const FCKeyIf &key);

private:
    uint32_t m_hashSize;
    HashBucket<FCVal> *m_hashMap;

    /** 闲置节点池
     * 使用栈结构存储是为了减少可能的swap分区的使用和优化CPU缓存的使用；
     * 但每次出入需要加锁，如果线程太多会在此处形成瓶颈；*/
    std::stack<FCNode<FCVal>*> m_idleNodes;
    pthread_mutex_t m_idleNodesLock;
};


////////////////////////////////////////////////////////////////////////////////


template<class FCVal>
FCache<FCVal>::FCache(uint32_t hashSize, uint32_t nodeCount) {
    // 分配hash数组内存
    m_hashSize = hashSize;
    m_hashMap = new HashBucket<FCVal>[m_hashSize];
    
    // 分配节点内存
    for (uint32_t i = 0; i < nodeCount; i++) {
        m_idleNodes.push(new FCNode<FCVal>);
    }
}


template<class FCVal>
FCache<FCVal>::~FCache() {
    // 释放hash数组内存
    delete [] m_hashMap;

    // 清空hash树

    // 释放节点内存
    while (!m_idleNodes.empty()) {
        FCNode<FCVal> *pNode = m_idleNodes.top();
        delete pNode;
        m_idleNodes.pop();
    }
}


template<class FCVal>
FCNode<FCVal>*
FCache<FCVal>::getNode(const FCKeyIf &key) {
    return NULL;
}


template<class FCVal>
FCNode<FCVal>*
FCache<FCVal>::insertNode(const FCKeyIf &key) {
    HashBucket<FCVal> bucket = m_hashMap[key.getHashVal(m_hashSize)];
    return bucket.insertNode(key);
}


#endif
