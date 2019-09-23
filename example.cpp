#include "f_cache.hpp"


class MyKey : public FCKeyIf
{
public:
    uint32_t m_val;

    MyKey(uint32_t val) { m_val = val; }
    
    uint32_t getHashVal(const uint32_t &hashSize) const {
        return m_val % hashSize;
    }
};


int main() {
    FCache<int> fcObj(100, 1000);
    fcObj.init();
    fcObj.start();

    MyKey myKey(12);
    FCNode<int> *pNode = NULL;

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
