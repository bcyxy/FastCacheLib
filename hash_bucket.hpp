#ifndef __HASH_BUCKET_H__
#define __HASH_BUCKET_H__


template<class FCKey, class FCVal> class FCache;


template<class FCKey, class FCVal>
struct FCNode
{
    FCKey m_key;
    FCVal m_val;
    pthread_mutex_t m_mutex;
    FCNode *m_pUpNode;    /**< 快查:父节点 */
    FCNode *m_pLNode;     /**< 快查:左子节点 */
    FCNode *m_pRNode;     /**< 快查:右子节点 */
    FCNode *m_pNextNode;  /**< 老化:后节点 */
    FCNode *m_pFrontNode; /**< 老化:前节点 */

    FCNode() {}
    ~FCNode() {}
    int initNode();  /**< 初始化节点 */
    int unlock() {}  /**< 解锁节点，使用结束后调用 */
    int delSelf() {} /**< 从hash树上删除节点，删除后节点被放入空闲栈 */
};


/** hash表的桶
 * 关键结构是一个满二叉树，所有的值存在叶子节点中；*/
template<class FCKey, class FCVal>
class HashBucket
{
public:
    HashBucket();
    ~HashBucket();

    int init(FCache<FCKey, FCVal> *pBelongCache);
    FCNode<FCKey, FCVal>* insertNode(const FCKey &key);

private:
    FCache<FCKey, FCVal> *m_pBelongCache;
    pthread_rwlock_t m_rwLock;
    FCNode<FCKey, FCVal> *m_treeTop;

    /** 通过key寻找节点
     * @param pNode
     *     输出找到的或最近的节点指针
     * @return 查找状态
     *     0: 已找到，pNode指向找到的节点；
     *     1: 未找到，pNode指向最近的节点（用于插入节点）；
     *     2: 未找到，空树；*/
    int __findNode(const FCKey &key, FCNode<FCKey, FCVal> *&pNode);
};


////////////////////////////////////////////////////////////////////////////////


template<class FCKey, class FCVal>
int
FCNode<FCKey, FCVal>::initNode() {
    m_pUpNode = NULL;
    m_pLNode = NULL;
    m_pRNode = NULL;
    m_pNextNode = NULL;
    m_pFrontNode = NULL;
}


template<class FCKey, class FCVal>
HashBucket<FCKey, FCVal>::HashBucket() {
    m_pBelongCache = NULL;
}


template<class FCKey, class FCVal>
HashBucket<FCKey, FCVal>::~HashBucket() {
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::init(FCache<FCKey, FCVal> *pBelongCache) {
    m_pBelongCache = pBelongCache;
    return 0;
}


template<class FCKey, class FCVal>
FCNode<FCKey, FCVal>*
HashBucket<FCKey, FCVal>::insertNode(const FCKey &key) {
    FCNode<FCKey, FCVal> *pNode = NULL;
    int rst = __findNode(key, pNode);
    if (rst == 2) {
        pNode = m_pBelongCache->getIdleNode();
        if (pNode == NULL) {
            return NULL;
        }
        m_treeTop = pNode;
        return m_treeTop;
    }
    return NULL;
}


template<class FCKey, class FCVal>
int
HashBucket<FCKey, FCVal>::__findNode(const FCKey &key, FCNode<FCKey, FCVal> *&pNode) {
    if (m_treeTop == NULL) {
        return 2;
    }

    FCNode<FCKey, FCVal> *tmpNodePtr = m_treeTop;
    while (1) {
        if (key < tmpNodePtr->m_key) {
            if (tmpNodePtr->m_pLNode == NULL) {
                pNode = tmpNodePtr;
                return 1;
            }
            tmpNodePtr = tmpNodePtr->m_pLNode;
            continue;
        }
        else if (key > tmpNodePtr->m_key) {
            if (tmpNodePtr->m_pRNode == NULL) {
                pNode = tmpNodePtr;
                return 1;
            }
            tmpNodePtr = tmpNodePtr->m_pRNode;
            continue;
        }
        else { // ==
            if (tmpNodePtr->m_pLNode == NULL) {
                pNode = tmpNodePtr;
                return 0;
            }
            tmpNodePtr = tmpNodePtr->m_pLNode;
            continue;
        }
    }
}


#endif
