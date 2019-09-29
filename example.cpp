#include <stdio.h>
#include "f_cache.hpp"


class MyKey
{
public:
    uint32_t m_val;
    MyKey() { m_val = 12; }
    uint32_t getHashVal(const uint32_t &hashSize) const {
        return m_val % hashSize;
    }
    bool operator < (const MyKey &obj2) const { return m_val < obj2.m_val; }
    bool operator > (const MyKey &obj2) const { return m_val > obj2.m_val; }
    bool operator == (const MyKey &obj2) const { return m_val == obj2.m_val; }
    bool operator <= (const MyKey &obj2) const { return m_val <= obj2.m_val; }
    bool operator >= (const MyKey &obj2) const { return m_val >= obj2.m_val; }
};


void oldCallBack(MyKey key, int val, void *argv) {
    printf("Old key=%u, val=%d\n", key.m_val, val);
}


int main() {
    FCache<MyKey, int> fcObj(100, 1000);
    fcObj.init(oldCallBack, NULL);
    fcObj.start();

    MyKey myKey;
    myKey.m_val = 12;
    if (fcObj.insert(myKey, 123) == 0) {
        printf("Insert value success. key=%u, size=%u\n",
               myKey.m_val, fcObj.size());
    }
    myKey.m_val = 23;
    if (fcObj.insert(myKey, 456) == 0) {
        printf("Insert value success. key=%u, size=%u\n",
               myKey.m_val, fcObj.size());
    }

    int val = 0;
    if (fcObj.get(myKey, val) == 0) {
        printf("Get value success. key=%u, val=%d\n", myKey.m_val, val);
    }

    if (fcObj.del(myKey) == 0) {
        printf("Delete success. key=%u, size=%u\n", myKey.m_val, fcObj.size());
    }

    sleep(10);
    printf("Size = %u\n", fcObj.size());
    fcObj.stop();
    return 0;
}
