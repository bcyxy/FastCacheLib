#ifndef __F_CACHE_H__
#define __F_CACHE_H__

#include <pthread.h>
#include <stack>
#include <stdint.h>
#include "hash_bucket.hpp"


/** Fast cache
 * 推荐用法：
 *     - 使用前预估元素数量，尽量大的设置hash数组长度，避免桶中有较多元素（桶的访问是加锁的）；*/
template<class FCKey, class FCVal>
class FCache
{
public:
    FCache(uint32_t hashSize, uint32_t nodeCount);
    ~FCache();
    int init() {}
    int start() {}
    int stop() {}
    FCNode<FCKey, FCVal>* getNode(const FCKey &key);
    FCNode<FCKey, FCVal>* insertNode(const FCKey &key);

    FCNode<FCKey, FCVal>* getIdleNode();          /**< 从节点池中获取节点 */
    bool returnNode(FCNode<FCKey, FCVal> *pNode); /**< 将节点归还到节点池中 */

private:
    uint32_t m_hashSize;
    HashBucket<FCKey, FCVal> *m_hashMap;

    /** 闲置节点池
     * 使用栈结构存储是为了减少可能的swap分区的使用和优化CPU缓存的使用；
     * 但每次出入需要加锁，如果线程太多会在此处形成瓶颈；*/
    std::stack<FCNode<FCKey, FCVal>*> m_idleNodesPool;
    pthread_mutex_t m_idleNodesPoolLock;
};


////////////////////////////////////////////////////////////////////////////////


template<class FCKey, class FCVal>
FCache<FCKey, FCVal>::FCache(uint32_t hashSize, uint32_t nodeCount) {
    pthread_mutex_init(&m_idleNodesPoolLock, NULL);

    // 分配hash数组内存
    m_hashSize = hashSize;
    m_hashMap = new HashBucket<FCKey, FCVal>[m_hashSize];
    for (uint32_t i = 0; i < m_hashSize; i++) {
        m_hashMap[i].init(this);
    }
    
    // 分配节点内存
    for (uint32_t i = 0; i < nodeCount; i++) {
        m_idleNodesPool.push(new FCNode<FCKey, FCVal>);
    }
}


template<class FCKey, class FCVal>
FCache<FCKey, FCVal>::~FCache() {
    pthread_mutex_destroy(&m_idleNodesPoolLock);

    // 释放hash数组内存
    delete [] m_hashMap;

    // 清空hash树

    // 释放节点内存
    while (!m_idleNodesPool.empty()) {
        FCNode<FCKey, FCVal> *pNode = m_idleNodesPool.top();
        delete pNode;
        m_idleNodesPool.pop();
    }
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>*
FCache<FCKey, FCVal>::getNode(const FCKey &key) {
    return NULL;
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>*
FCache<FCKey, FCVal>::insertNode(const FCKey &key) {
    HashBucket<FCKey, FCVal> bucket = m_hashMap[key.getHashVal(m_hashSize)];
    return bucket.insertNode(key);
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>*
FCache<FCKey, FCVal>::getIdleNode() {
    FCNode<FCKey, FCVal> *pNode = NULL;
    pthread_mutex_lock(&m_idleNodesPoolLock);
    if (!m_idleNodesPool.empty()) {
        pNode = m_idleNodesPool.top();
        m_idleNodesPool.pop();
    }
    pthread_mutex_unlock(&m_idleNodesPoolLock);
    pNode->initNode();
    return pNode;
}


template<class FCKey, class FCVal>
bool
FCache<FCKey, FCVal>::returnNode(FCNode<FCKey, FCVal> *pNode) {
    return false;
}


#endif
