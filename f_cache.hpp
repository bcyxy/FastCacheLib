#ifndef __F_CACHE_H__
#define __F_CACHE_H__

#include <map>
#include <pthread.h>
#include <stack>
#include <stdint.h>
#include <unistd.h>


using namespace std;


template<class FCKey, class FCVal> class FCache;


template<class FCKey, class FCVal>
struct FCNode
{
    FCKey m_key;
    FCVal m_val;
    FCNode *m_pNextNode;  /**< 老化:后节点 */
    FCNode *m_pFrontNode; /**< 老化:前节点 */
    time_t m_tm;

    FCNode();
    ~FCNode();
    int init();
};


template<class FCKey, class FCVal>
class HashBucket
{
public:
    #define BucketMap map<FCKey, FCNode<FCKey, FCVal>*>
    #define BucketMapIt typename BucketMap::iterator
    
    HashBucket();
    ~HashBucket();
    int init(FCache<FCKey, FCVal> *pBelong);
    /** 同FCache::get */
    int get(const FCKey &key, FCVal &outVal);
    /** 同FCache::insert */
    int insert(const FCKey &key, const FCVal &val);
    /** 同FCache::del */
    int del(const FCKey &key);
    /** 删除老化的节点 */
    int delOld(time_t currTm);
    uint32_t size() { return m_bucketMap.size(); };

private:
    pthread_mutex_t m_bucketLock;
    BucketMap m_bucketMap;
    FCache<FCKey, FCVal> *m_pBelong;
    
    ////// 老化链表，从前向后老化 ////////////////////////////////////////////////
    FCNode<FCKey, FCVal> *m_oldLinkHead;
    FCNode<FCKey, FCVal> *m_oldLinkTail;
    void __oldLinkAddNode(FCNode<FCKey, FCVal> *pNode); /**< 向老化链增加节点 */
    void __oldLinkDelNode(FCNode<FCKey, FCVal> *pNode); /**< 从老化链摘除节点 */
};


/** Fast cache */
template<class FCKey, class FCVal>
class FCache
{
public:
    friend class HashBucket<FCKey, FCVal>;

    FCache(uint32_t hashSize, uint32_t maxNodeCount);
    ~FCache();
    int init(void (*pOldCallBack)(FCKey key, FCVal val, void *argv),
             void *oldCallArgv=NULL);
    int start();
    int stop();
    /** 获取值
     * @param outVal
     *     输出查到的值
     * @return
     *     0: 正常获取
     *     1: 此值不存在 */
    int get(const FCKey &key, FCVal &outVal);
    /** 插入值
     * @return
     *     0: 正常插入
     *     1: 此值已经存在
     *     2: 节点已经用完 */
    int insert(const FCKey &key, const FCVal &val);
    /** 删除值
     * @return
     *     0: 正常删除
     *     1: 此值不存在 */
    int del(const FCKey &key);
    /** 获取元素个数 */
    uint32_t size();
    /** 设置老化时间，单位：秒 */
    int setOldTime(uint32_t oldSec) { m_oldSec = oldSec; }

private:
    uint32_t m_hashSize;
    HashBucket<FCKey, FCVal> *m_hashMap;

    ////// 老化 ////////////////////////////////////////////////////////////////
    pthread_t m_oldTid;
    void *m_oldCallArgv;
    uint32_t m_oldSec; /**< 老化的秒数 */
    /** 老化后的回调函数 */
    void (*m_pOldCallBack)(FCKey key, FCVal val, void *argv);
    void __oldLoop();
    static void* __threadFunc(void *pObj);

    ////// 节点池 //////////////////////////////////////////////////////////////
    std::stack<FCNode<FCKey, FCVal>*> m_idleNodesPool;
    pthread_mutex_t m_idleNodesPoolLock;
    FCNode<FCKey, FCVal>* __applyNode();           /**< 从节点池申请节点 */
    int __returnNode(FCNode<FCKey, FCVal> *pNode); /**< 向节点池归还节点 */

    ////// 时间服务，避免频繁获取系统时间的耗时 ///////////////////////////////////
    time_t m_currTm;
    pthread_rwlock_t m_currTmLock;
    time_t __getTm();
    void __updateTm();
};


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>::FCNode() {
    init();
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>::~FCNode() {
}


template<class FCKey, class FCVal>
int
FCNode<FCKey, FCVal>::init() {
    m_pNextNode = NULL;
    m_pFrontNode = NULL;
}


template<class FCKey, class FCVal>
HashBucket<FCKey, FCVal>::HashBucket() {
    pthread_mutex_init(&m_bucketLock, NULL);
}


template<class FCKey, class FCVal>
HashBucket<FCKey, FCVal>::~HashBucket() {
    pthread_mutex_destroy(&m_bucketLock);
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::init(FCache<FCKey, FCVal> *pBelong) {
    m_pBelong = pBelong;
    m_bucketMap.clear();
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::get(const FCKey &key, FCVal &outVal) {
    int rst = 0;
    FCNode<FCKey, FCVal> *pNode = NULL;
    pthread_mutex_lock(&m_bucketLock);
    BucketMapIt bIt = m_bucketMap.find(key);
    if (bIt != m_bucketMap.end()) {
        pNode = bIt->second;
        outVal = pNode->m_val;
        __oldLinkDelNode(pNode);
        __oldLinkAddNode(pNode);
    }
    else {
        rst = 1;
    }
    pthread_mutex_unlock(&m_bucketLock);
    return rst;
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::insert(const FCKey &key, const FCVal &val) {
    int rst = 0;
    FCNode<FCKey, FCVal> *pNode = NULL;
    pthread_mutex_lock(&m_bucketLock);
    BucketMapIt bIt = m_bucketMap.find(key);
    if (bIt != m_bucketMap.end()) {
        pNode = bIt->second;
        __oldLinkDelNode(pNode);
        __oldLinkAddNode(pNode);
        rst = 1;
    }
    else {
        pNode = m_pBelong->__applyNode();
        if (pNode == NULL) {
            rst = 2;
        }
        else {
            pNode->m_key = key;
            pNode->m_val = val;
            pNode->m_tm = m_pBelong->__getTm();
            pair<BucketMapIt, bool> iRst = m_bucketMap.insert(make_pair(key, pNode));
            __oldLinkAddNode(pNode);
        }
    }
    pthread_mutex_unlock(&m_bucketLock);
    return rst;
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::del(const FCKey &key) {
    int rst = 0;
    FCNode<FCKey, FCVal> *pNode = NULL;
    pthread_mutex_lock(&m_bucketLock);
    BucketMapIt bIt = m_bucketMap.find(key);
    if (bIt == m_bucketMap.end()) {
        rst = 1;
    }
    else {
        pNode = bIt->second;
        __oldLinkDelNode(pNode);
        m_bucketMap.erase(bIt);
        m_pBelong->__returnNode(pNode);
    }
    pthread_mutex_unlock(&m_bucketLock);
    return rst;
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::delOld(time_t currTm) {
    int delCounter = 0;
    pthread_mutex_lock(&m_bucketLock);
    while (1) {
        if (m_oldLinkHead == NULL) {
            break;
        }

        if (currTm - m_oldLinkHead->m_tm > m_pBelong->m_oldSec) {
            FCNode<FCKey, FCVal> *pNode = m_oldLinkHead;
            m_oldLinkHead = m_oldLinkHead->m_pNextNode;
            if (m_oldLinkHead != NULL) {
                m_oldLinkHead->m_pFrontNode = NULL;
            }
            m_bucketMap.erase(pNode->m_key);
            if (m_pBelong->m_pOldCallBack) {
                m_pBelong->m_pOldCallBack(pNode->m_key, pNode->m_val,
                                          m_pBelong->m_oldCallArgv);
            }
            m_pBelong->__returnNode(pNode);
        }
        else {
            break;
        }
    }
    pthread_mutex_unlock(&m_bucketLock);
    return delCounter;
}


template<class FCKey, class FCVal>
void
HashBucket<FCKey, FCVal>::__oldLinkAddNode(FCNode<FCKey, FCVal> *pNode) {
    pNode->m_tm = m_pBelong->__getTm();
    pNode->m_pNextNode = NULL;
    pNode->m_pFrontNode = m_oldLinkTail;
    if (m_oldLinkTail == m_oldLinkHead) {
        m_oldLinkHead = pNode;
    }
    else {
        m_oldLinkTail->m_pNextNode = pNode;
    }
    m_oldLinkTail = pNode;
}


template<class FCKey, class FCVal>
void
HashBucket<FCKey, FCVal>::__oldLinkDelNode(FCNode<FCKey, FCVal> *pNode) {
    if (pNode->m_pFrontNode == NULL) {
        m_oldLinkHead = pNode->m_pNextNode;
    }
    else {
        pNode->m_pFrontNode->m_pNextNode = pNode->m_pNextNode;
    }

    if (pNode->m_pNextNode == NULL) {
        m_oldLinkTail = pNode->m_pFrontNode;
    }
    else {
        pNode->m_pNextNode->m_pFrontNode = pNode->m_pFrontNode;
    }
}


template<class FCKey, class FCVal>
FCache<FCKey, FCVal>::FCache(uint32_t hashSize, uint32_t nodeCount) {
    pthread_mutex_init(&m_idleNodesPoolLock, NULL);
    pthread_rwlock_init(&m_currTmLock, NULL);

    // 分配hash数组内存
    m_hashSize = hashSize;
    m_hashMap = new HashBucket<FCKey, FCVal>[m_hashSize];

    // 分配节点内存
    for (uint32_t i = 0; i < nodeCount; i++) {
        m_idleNodesPool.push(new FCNode<FCKey, FCVal>);
    }

    init(NULL);
}


template<class FCKey, class FCVal>
FCache<FCKey, FCVal>::~FCache() {
    init(NULL);

    pthread_mutex_destroy(&m_idleNodesPoolLock);
    pthread_rwlock_destroy(&m_currTmLock);
    
    // 释放hash数组内存
    delete [] m_hashMap;

    // 释放节点内存
    while (!m_idleNodesPool.empty()) {
        FCNode<FCKey, FCVal> *pNode = m_idleNodesPool.top();
        delete pNode;
        m_idleNodesPool.pop();
    }
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::init(void (*pOldCallBack)(FCKey key, FCVal val, void *argv),
                           void *oldCallArgv)
{
    // 清空索引
    for (uint32_t i = 0; i < m_hashSize; i++) {
        m_hashMap[i].init(this);
    }
    m_pOldCallBack = pOldCallBack;
    m_oldCallArgv = oldCallArgv;
    m_oldSec = 5;
    time(&m_currTm);
    return 0;
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::start() {
    pthread_create(&m_oldTid, NULL, __threadFunc, (void*)this);
    return 0;
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::stop() {
    pthread_cancel(m_oldTid);
    return 0;
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::get(const FCKey &key, FCVal &outVal) {
    uint32_t hashIndex = key.getHashVal(m_hashSize);
    HashBucket<FCKey, FCVal> &bucket = m_hashMap[hashIndex];
    return bucket.get(key, outVal);
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::insert(const FCKey &key, const FCVal &val) {
    uint32_t hashIndex = key.getHashVal(m_hashSize);
    HashBucket<FCKey, FCVal> &bucket = m_hashMap[hashIndex];
    return bucket.insert(key, val);
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::del(const FCKey &key) {
    uint32_t hashIndex = key.getHashVal(m_hashSize);
    HashBucket<FCKey, FCVal> &bucket = m_hashMap[hashIndex];
    return bucket.del(key);
}


template<class FCKey, class FCVal>
uint32_t
FCache<FCKey, FCVal>::size() {
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < m_hashSize; i++) {
        totalSize += m_hashMap[i].size();
    }
    return totalSize;
}


template<class FCKey, class FCVal>
void
FCache<FCKey, FCVal>::__oldLoop() {
    while (1) {
        sleep(1);
        __updateTm();
        for (uint32_t i = 0; i < m_hashSize; i++) {
            m_hashMap[i].delOld(__getTm());
        }
    }
}


template<class FCKey, class FCVal>
void*
FCache<FCKey, FCVal>::__threadFunc(void *pObj) {
    ((FCache<FCKey, FCVal>*)pObj)->__oldLoop();
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>*
FCache<FCKey, FCVal>::__applyNode() {
    FCNode<FCKey, FCVal>* pNode  = NULL;
    pthread_mutex_lock(&m_idleNodesPoolLock);
    if (!m_idleNodesPool.empty()) {
        pNode = m_idleNodesPool.top();
        pNode->init();
        m_idleNodesPool.pop();
    }
    pthread_mutex_unlock(&m_idleNodesPoolLock);
    return pNode;
}


template<class FCKey, class FCVal>
int
FCache<FCKey, FCVal>::__returnNode(FCNode<FCKey, FCVal>* pNode) {
    pthread_mutex_lock(&m_idleNodesPoolLock);
    m_idleNodesPool.push(pNode);
    pthread_mutex_unlock(&m_idleNodesPoolLock);
    return 0;
}


template<class FCKey, class FCVal>
time_t
FCache<FCKey, FCVal>::__getTm() {
    time_t tm;
    pthread_rwlock_rdlock(&m_currTmLock);
    tm = m_currTm;
    pthread_rwlock_unlock(&m_currTmLock);
    return tm;
}


template<class FCKey, class FCVal>
void
FCache<FCKey, FCVal>::__updateTm() {
    pthread_rwlock_wrlock(&m_currTmLock);
    time(&m_currTm);
    pthread_rwlock_unlock(&m_currTmLock);
}


#endif
