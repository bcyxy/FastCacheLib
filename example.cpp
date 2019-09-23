#include "f_cache.hpp"


class MyKey : public FCKeyIf
{
public:
    uint32_t getHashVal() {
        return 0;
    }
};


int main() {
    FCache<int> fcObj;
    fcObj.init();
    fcObj.start();

    MyKey myKey;
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
