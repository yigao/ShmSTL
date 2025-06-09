// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashTableEraseAdvanced.h
//    @Author           :    gaoyi
//    @Date             :    2025/1/27
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashTableEraseAdvanced
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashTable.h"
#include <string>
#include <functional>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>

// Basic class definitions (compatible with TestNFShmHashTable.h)
struct EraseAdvanceHashFunc
{
    size_t operator()(int key) const
    {
        return std::hash<int>{}(key);
    }
};

// Conflict hash function (intentionally create conflicts)
struct EraseAdvanceConflictHashFunc
{
    size_t operator()(int key) const
    {
        // Intentionally create more conflicts to test linked list operations
        return key % 10;
    }
};

// Advanced test key-value pair structure (with constructor/destructor counting)
struct AdvancedTestPair
{
    int key;
    std::string value;
    static int constructor_count;
    static int destructor_count;

    AdvancedTestPair() : key(0), value("")
    {
        constructor_count++;
    }

    AdvancedTestPair(int k, const std::string& v) : key(k), value(v)
    {
        constructor_count++;
    }

    AdvancedTestPair(const AdvancedTestPair& other) : key(other.key), value(other.value)
    {
        constructor_count++;
    }

    ~AdvancedTestPair()
    {
        destructor_count++;
    }

    AdvancedTestPair& operator=(const AdvancedTestPair& other)
    {
        if (this != &other)
        {
            key = other.key;
            value = other.value;
        }
        return *this;
    }

    bool operator==(const AdvancedTestPair& other) const
    {
        return key == other.key && value == other.value;
    }

    static void ResetCounters()
    {
        constructor_count = 0;
        destructor_count = 0;
    }

    static int GetConstructorCount() { return constructor_count; }
    static int GetDestructorCount() { return destructor_count; }
};

int AdvancedTestPair::constructor_count = 0;
int AdvancedTestPair::destructor_count = 0;

// Key extraction function object (adapted for AdvancedTestPair)
struct AdvancedExtractKey
{
    int operator()(const AdvancedTestPair& pair) const
    {
        return pair.key;
    }
};

// Key comparison function object (adapted for int keys)
struct AdvancedEqualKey
{
    bool operator()(int key1, int key2) const
    {
        return key1 == key2;
    }
};

// Special hash function: all keys map to the same bucket
struct SingleBucketHash
{
    size_t operator()(int key) const
    {
        return 0; // All keys hash to bucket 0
    }
};

// Define hash table types for testing
using AdvancedTestHashTable = NFShmHashTable<AdvancedTestPair, int, 100, EraseAdvanceHashFunc, AdvancedExtractKey, AdvancedEqualKey>;
using ConflictTestHashTable = NFShmHashTable<AdvancedTestPair, int, 50, EraseAdvanceConflictHashFunc, AdvancedExtractKey, AdvancedEqualKey>;

// Helper class: automatic counter reset
class CounterResetter
{
public:
    CounterResetter()
    {
        AdvancedTestPair::ResetCounters();
    }

    ~CounterResetter()
    {
        AdvancedTestPair::ResetCounters();
    }
};

// =========================== Advanced erase tests ===========================

// Test erasing large number of conflicting keys
TEST(NFShmHashTableEraseAdvancedTest, EraseWithManyCollisions)
{
    CounterResetter resetter;
    ConflictTestHashTable ht;

    // Insert large number of keys that will cause conflicts
    std::vector<int> keys;
    for (int i = 0; i < 30; ++i)
    {
        keys.push_back(i * 10); // These keys will cause conflicts under ConflictHashFunc
    }

    // Insert all keys
    for (int key : keys)
    {
        AdvancedTestPair pair(key, "value" + std::to_string(key));
        auto result = ht.insert_unique(pair);
        ASSERT_TRUE(result.second);
    }

    ASSERT_EQ(ht.size(), keys.size());

    // Randomly erase half of the keys
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    std::vector<int> keysToDelete(keys.begin(), keys.begin() + keys.size() / 2);
    std::vector<int> keysToKeep(keys.begin() + keys.size() / 2, keys.end());

    for (int key : keysToDelete)
    {
        auto iter = ht.find(key);
        ASSERT_NE(iter, ht.end());

        auto nextIter = ht.erase(iter);
        EXPECT_EQ(ht.find(key), ht.end());
    }

    EXPECT_EQ(ht.size(), keysToKeep.size());

    // Verify remaining keys still exist
    for (int key : keysToKeep)
    {
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, key);
        EXPECT_EQ(iter->value, "value" + std::to_string(key));
    }

    // Verify erased keys really don't exist
    for (int key : keysToDelete)
    {
        EXPECT_EQ(ht.find(key), ht.end());
    }
}

// Test memory leak detection
TEST(NFShmHashTableEraseAdvancedTest, MemoryLeakDetection)
{
    CounterResetter resetter;

    {
        AdvancedTestHashTable ht;

        // Insert elements
        for (int i = 1; i <= 20; ++i)
        {
            AdvancedTestPair pair(i, "value" + std::to_string(i));
            ht.insert_unique(pair);
        }

        int constructorCountAfterInsert = AdvancedTestPair::GetConstructorCount();
        int destructorCountAfterInsert = AdvancedTestPair::GetDestructorCount();

        // Erase half of the elements
        for (int i = 1; i <= 10; ++i)
        {
            size_t erased = ht.erase(i);
            EXPECT_EQ(erased, 1);
        }

        int destructorCountAfterErase = AdvancedTestPair::GetDestructorCount();

        // Verify destructors were called correctly
        EXPECT_EQ(destructorCountAfterErase - destructorCountAfterInsert, 10);

        // Clear remaining elements
        ht.clear();

        int destructorCountAfterClear = AdvancedTestPair::GetDestructorCount();

        // Verify all elements were properly destructed
        EXPECT_GE(destructorCountAfterClear - destructorCountAfterInsert, 20);
    }

    // After scope ends, all temporary objects should also be destructed
}

// Test recursive erase (from begin to end)
TEST(NFShmHashTableEraseAdvancedTest, RecursiveEraseFromBeginToEnd)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    // Insert test data
    std::vector<int> originalKeys;
    for (int i = 1; i <= 15; ++i)
    {
        originalKeys.push_back(i * 10);
        AdvancedTestPair pair(i * 10, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 15);

    // Erase one by one from begin until table is empty
    while (!ht.empty())
    {
        auto iter = ht.begin();
        ASSERT_NE(iter, ht.end());

        int keyToDelete = iter->key;
        auto nextIter = ht.erase(iter);

        // Verify element is erased
        EXPECT_EQ(ht.find(keyToDelete), ht.end());

        // Verify size decremented by 1
        EXPECT_EQ(ht.size(), originalKeys.size() - 1);
        originalKeys.erase(std::find(originalKeys.begin(), originalKeys.end(), keyToDelete));

        // Verify remaining elements still exist
        for (int key : originalKeys)
        {
            EXPECT_NE(ht.find(key), ht.end());
        }
    }

    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.size(), 0);
    EXPECT_EQ(ht.begin(), ht.end());
}

// Test reverse erase (from tail to head)
TEST(NFShmHashTableEraseAdvancedTest, ReverseErase)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    // Insert test data
    std::vector<int> keys = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int key : keys)
    {
        AdvancedTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 10);

    // Reverse erase (by finding the largest key to simulate erasing from tail)
    while (!ht.empty())
    {
        // Find current largest key
        int maxKey = -1;
        for (auto iter = ht.begin(); iter != ht.end(); ++iter)
        {
            if (iter->key > maxKey)
            {
                maxKey = iter->key;
            }
        }

        ASSERT_GT(maxKey, 0);

        auto iter = ht.find(maxKey);
        ASSERT_NE(iter, ht.end());

        ht.erase(iter);
        EXPECT_EQ(ht.find(maxKey), ht.end());

        // Remove this key from keys vector
        keys.erase(std::find(keys.begin(), keys.end(), maxKey));

        // Verify remaining elements
        for (int key : keys)
        {
            EXPECT_NE(ht.find(key), ht.end());
        }
    }

    EXPECT_TRUE(ht.empty());
}

// Test erase and immediate reinsert of same key
TEST(NFShmHashTableEraseAdvancedTest, EraseAndImmediateReinsert)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    // Initial insert
    std::vector<int> keys = {10, 20, 30, 40, 50};
    for (int key : keys)
    {
        AdvancedTestPair pair(key, "original" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 5);

    // Perform erase-reinsert operation for each key
    for (int key : keys)
    {
        // Erase
        auto iter = ht.find(key);
        ASSERT_NE(iter, ht.end());
        EXPECT_EQ(iter->value, "original" + std::to_string(key));

        ht.erase(iter);
        EXPECT_EQ(ht.find(key), ht.end());
        EXPECT_EQ(ht.size(), 4);

        // Immediately reinsert with different value
        AdvancedTestPair newPair(key, "new" + std::to_string(key));
        auto result = ht.insert_unique(newPair);
        EXPECT_TRUE(result.second);
        EXPECT_EQ(result.first->key, key);
        EXPECT_EQ(result.first->value, "new" + std::to_string(key));
        EXPECT_EQ(ht.size(), 5);

        // Verify other keys are not affected
        for (int otherKey : keys)
        {
            if (otherKey != key)
            {
                auto otherIter = ht.find(otherKey);
                EXPECT_NE(otherIter, ht.end());
            }
        }
    }

    // Final verification that all keys have new values
    for (int key : keys)
    {
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->value, "new" + std::to_string(key));
    }
}

// Test simultaneous insert and erase operations
TEST(NFShmHashTableEraseAdvancedTest, InterleavedInsertAndErase)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    std::vector<int> activeKeys;

    for (int round = 0; round < 10; ++round)
    {
        // Insert some new keys
        for (int i = 0; i < 5; ++i)
        {
            int newKey = round * 100 + i;
            AdvancedTestPair pair(newKey, "value" + std::to_string(newKey));
            auto result = ht.insert_unique(pair);
            if (result.second)
            {
                activeKeys.push_back(newKey);
            }
        }

        // Erase some existing keys
        if (activeKeys.size() > 3)
        {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(activeKeys.begin(), activeKeys.end(), g);

            for (int i = 0; i < 2 && !activeKeys.empty(); ++i)
            {
                int keyToDelete = activeKeys.back();
                activeKeys.pop_back();

                auto iter = ht.find(keyToDelete);
                EXPECT_NE(iter, ht.end());
                ht.erase(iter);
                EXPECT_EQ(ht.find(keyToDelete), ht.end());
            }
        }

        // Verify current state
        EXPECT_EQ(ht.size(), activeKeys.size());

        for (int key : activeKeys)
        {
            auto iter = ht.find(key);
            EXPECT_NE(iter, ht.end());
            EXPECT_EQ(iter->key, key);
        }
    }
}

// Test boundary conditions: erase under capacity limits
TEST(NFShmHashTableEraseAdvancedTest, EraseUnderCapacityLimits)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    // Fill hash table to capacity
    for (int i = 0; i < 100; ++i)
    {
        AdvancedTestPair pair(i, "value" + std::to_string(i));
        auto result = ht.insert_unique(pair);
        ASSERT_TRUE(result.second);
    }

    ASSERT_TRUE(ht.full());
    ASSERT_EQ(ht.size(), 100);
    ASSERT_EQ(ht.left_size(), 0);

    // Try to insert additional element (should fail)
    AdvancedTestPair extraPair(999, "extra");
    auto result = ht.insert_unique(extraPair);
    EXPECT_FALSE(result.second);

    // Erase some elements to free space
    std::vector<int> keysToDelete = {5, 15, 25, 35, 45};
    for (int key : keysToDelete)
    {
        size_t erased = ht.erase(key);
        EXPECT_EQ(erased, 1);
    }

    EXPECT_FALSE(ht.full());
    EXPECT_EQ(ht.size(), 95);
    EXPECT_EQ(ht.left_size(), 5);

    // Now should be able to insert new elements
    for (int i = 0; i < 5; ++i)
    {
        AdvancedTestPair newPair(1000 + i, "new" + std::to_string(i));
        auto result = ht.insert_unique(newPair);
        EXPECT_TRUE(result.second);
    }

    EXPECT_TRUE(ht.full());
    EXPECT_EQ(ht.size(), 100);
    EXPECT_EQ(ht.left_size(), 0);
}

// Test performance: large number of random erase operations
TEST(NFShmHashTableEraseAdvancedTest, RandomErasePerformance)
{
    CounterResetter resetter;
    AdvancedTestHashTable ht;

    // Insert large amount of data
    const int dataSize = 80;
    std::vector<int> allKeys;
    for (int i = 1; i <= dataSize; ++i)
    {
        allKeys.push_back(i);
        AdvancedTestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), dataSize);

    // Random erase operations
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(allKeys.begin(), allKeys.end(), g);

    auto start = std::chrono::high_resolution_clock::now();

    // Erase 80% of the data
    int deleteCount = static_cast<int>(dataSize * 0.8);
    for (int i = 0; i < deleteCount; ++i)
    {
        int keyToDelete = allKeys[i];

        auto iter = ht.find(keyToDelete);
        ASSERT_NE(iter, ht.end());

        ht.erase(iter);
        EXPECT_EQ(ht.find(keyToDelete), ht.end());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(ht.size(), dataSize - deleteCount);

    // Verify remaining elements
    for (int i = deleteCount; i < dataSize; ++i)
    {
        int key = allKeys[i];
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, key);
    }

    // Performance requirement: erase operations should complete within reasonable time
    EXPECT_LT(duration.count(), 10000); // Less than 10ms

    std::cout << "Random erase performance: " << deleteCount
        << " operations in " << duration.count() << " microseconds" << std::endl;
}

// Test complex multi-value erase scenarios
TEST(NFShmHashTableEraseAdvancedTest, ComplexMultiValueErase)
{
    CounterResetter resetter;

    // Use version that allows duplicate keys
    ConflictTestHashTable ht; // Use conflict hash to increase complexity

    // Insert multiple values with same key
    std::vector<std::string> values = {"val1", "val2", "val3", "val4", "val5"};
    int commonKey = 100;

    for (const auto& val : values)
    {
        AdvancedTestPair pair(commonKey, val);
        ht.insert_equal(pair);
    }

    // Insert some values with other keys
    for (int i = 1; i <= 5; ++i)
    {
        AdvancedTestPair pair(i * 10, "other" + std::to_string(i));
        ht.insert_equal(pair);
    }

    ASSERT_EQ(ht.size(), 10);
    ASSERT_EQ(ht.count(commonKey), 5);

    // Erase elements with same key one by one
    auto range = ht.equal_range(commonKey);
    std::vector<ConflictTestHashTable::iterator> iterators;

    for (auto iter = range.first; iter != range.second; ++iter)
    {
        iterators.push_back(iter);
    }

    ASSERT_EQ(iterators.size(), 5);

    // Erase first three
    for (int i = 0; i < 3; ++i)
    {
        auto nextIter = ht.erase(iterators[i]);
        // Verify state after erase
        EXPECT_EQ(ht.count(commonKey), 5 - (i + 1));
    }

    EXPECT_EQ(ht.size(), 7);
    EXPECT_EQ(ht.count(commonKey), 2);

    // Verify other keys are not affected
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_EQ(ht.count(i * 10), 1);
    }

    // Erase remaining elements with same key
    size_t remainingErased = ht.erase(commonKey);
    EXPECT_EQ(remainingErased, 2);
    EXPECT_EQ(ht.size(), 5);
    EXPECT_EQ(ht.count(commonKey), 0);
}

// Test extreme case: single chain scenario
TEST(NFShmHashTableEraseAdvancedTest, SingleChainScenario)
{
    CounterResetter resetter;

    // Create a special hash table where all elements hash to the same bucket
    using SingleChainHashTable = NFShmHashTable<AdvancedTestPair, int, 20, SingleBucketHash, AdvancedExtractKey, AdvancedEqualKey>;

    SingleChainHashTable ht;

    // Insert multiple elements that will all form a long chain in the same bucket
    std::vector<int> keys = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int key : keys)
    {
        AdvancedTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 10);

    // Verify all elements are in the same bucket
    EXPECT_EQ(ht.elems_in_bucket(0), 10);
    for (size_t i = 1; i < 20; ++i)
    {
        EXPECT_EQ(ht.elems_in_bucket(i), 0);
    }

    // Test erasing from middle of chain
    auto iter = ht.find(5);
    ASSERT_NE(iter, ht.end());
    ht.erase(iter);

    EXPECT_EQ(ht.size(), 9);
    EXPECT_EQ(ht.find(5), ht.end());
    EXPECT_EQ(ht.elems_in_bucket(0), 9);

    // Verify chain structure is still intact
    for (int key : keys)
    {
        if (key != 5)
        {
            auto findIter = ht.find(key);
            EXPECT_NE(findIter, ht.end());
            EXPECT_EQ(findIter->key, key);
        }
    }

    // Test erasing chain head
    auto headIter = ht.begin();
    ASSERT_NE(headIter, ht.end());
    int headKey = headIter->key;

    ht.erase(headIter);
    EXPECT_EQ(ht.size(), 8);
    EXPECT_EQ(ht.find(headKey), ht.end());

    // Continue erasing until only one element remains
    while (ht.size() > 1)
    {
        auto iter = ht.begin();
        ASSERT_NE(iter, ht.end());
        ht.erase(iter);
    }

    EXPECT_EQ(ht.size(), 1);
    EXPECT_EQ(ht.elems_in_bucket(0), 1);

    // Erase last element
    auto lastIter = ht.begin();
    ASSERT_NE(lastIter, ht.end());
    ht.erase(lastIter);

    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.size(), 0);
    EXPECT_EQ(ht.elems_in_bucket(0), 0);
    EXPECT_EQ(ht.begin(), ht.end());
}
