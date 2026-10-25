#include <gtest/gtest.h>
using namespace std;

extern "C"{
    #include "memory.h"
}

TEST(MemoryTest, NormalAllocation){
    void* ptr = memory_alloc(10);
    ASSERT_NE(ptr , nullptr);
    memory_free(ptr);
}

TEST(MemoryTest, zeroSizeAllocation){
    ASSERT_EQ(memory_alloc(0) , nullptr);
}

TEST(MemoryTest, memoryOutstanding){
    void* ptr = memory_alloc(10);
    memory_free(ptr);
    ASSERT_EQ(memory_outstanding(), 0);
}