#include "f_cache.hpp"


class MyKey
{
public:
    uint32_t m_val;

    MyKey() { m_val = 12; }
    
    uint32_t getHashVal(const uint32_t &hashSize) const {
        return m_val % hashSize;
    }

    bool operator < (const MyKey &obj2) const {
        return m_val < obj2.m_val;
    }

    bool operator > (const MyKey &obj2) const {
        return m_val > obj2.m_val;
    }

    bool operator == (const MyKey &obj2) const {
        return m_val == obj2.m_val;
    }

    bool operator <= (const MyKey &obj2) const {
        return m_val <= obj2.m_val;
    }

    bool operator >= (const MyKey &obj2) const {
        return m_val >= obj2.m_val;
    }
};


int main() {
    FCache<MyKey, int> fcObj(100, 1000);
    fcObj.init();
    fcObj.start();

    MyKey myKey;
    FCNode<MyKey, int> *pNode = NULL;

    // 插入
    pNode = fcObj.insertNode(myKey);
    if (pNode != NULL) {
        pNode->m_val = 123;
        pNode->unlock();
    }

    // 查找
    pNode = fcObj.getNode(myKey);
    if (pNode != NULL) {
        pNode->m_val = 456;
        pNode->unlock();
    }

    // 删除
    pNode = fcObj.getNode(myKey);
    if (pNode != NULL) {
        pNode->delSelf();
        pNode->unlock();
    }

    fcObj.stop();
    return 0;
}
