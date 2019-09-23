#ifndef __HASH_BUCKET_H__
#define __HASH_BUCKET_H__


class FCKeyIf
{
public:
    virtual uint32_t getHashVal(const uint32_t &hashSize) const = 0;
};


template<class FCVal>
class FCNode
{
public:
    FCVal m_val;

    FCNode() {}
    ~FCNode() {}
    int initNode() {}
    int unlock() {}  /**< 使用结束后调用 */
    int delSelf() {} /**< 从hash树上删除节点，删除后节点被放入空闲栈 */

protected:
    pthread_mutex_t m_mutex;

private:
    // 用于索引
    FCNode *m_pUpNode;
    FCNode *m_pLNode;
    FCNode *m_pRNode;

    // 用于老化
    FCNode *m_pNextNode;
    FCNode *m_pFrontNode;
};


/***/
template<class FCVal>
class HashBucket
{
public:
    HashBucket() {}
    ~HashBucket() {}

    FCNode<FCVal>* insertNode(const FCKeyIf &key);
private:
    pthread_rwlock_t rw_lock;
    FCNode<FCVal> *m_treeTop;

    FCNode<FCVal> *m_tmpNodePtr;
};


////////////////////////////////////////////////////////////////////////////////


template<class FCVal>
FCNode<FCVal>*
HashBucket<FCVal>::insertNode(const FCKeyIf &key) {
    return NULL;
}


#endif
