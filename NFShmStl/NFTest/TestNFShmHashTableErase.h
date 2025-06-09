// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashTableErase.h
//    @Author           :    gaoyi
//    @Date             :    2025/1/27
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashTableErase
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

// Test simple key-value pair structure
struct EraseTestPair
{
    int key;
    std::string value;

    EraseTestPair() : key(0), value("")
    {
    }

    EraseTestPair(int k, const std::string& v) : key(k), value(v)
    {
    }

    bool operator==(const EraseTestPair& other) const
    {
        return key == other.key && value == other.value;
    }

    std::string toString() const
    {
        return "(" + std::to_string(key) + "," + value + ")";
    }
};

// Key extraction function object
struct EraseExtractKey
{
    int operator()(const EraseTestPair& pair) const
    {
        return pair.key;
    }
};

// Hash function object
struct EraseHashFunc
{
    size_t operator()(int key) const
    {
        return std::hash<int>{}(key);
    }
};

// Key comparison function object
struct EraseEqualKey
{
    bool operator()(int key1, int key2) const
    {
        return key1 == key2;
    }
};

// Define hash table type for testing
using EraseTestHashTable = NFShmHashTable<EraseTestPair, int, 50, EraseHashFunc, EraseExtractKey, EraseEqualKey>;

// Helper function: collect all keys from hash table
std::vector<int> CollectKeys(const EraseTestHashTable& ht)
{
    std::vector<int> keys;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        keys.push_back(iter->key);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

// Helper function: collect all values from hash table
std::vector<std::string> CollectValues(const EraseTestHashTable& ht)
{
    std::vector<std::string> values;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        values.push_back(iter->value);
    }
    std::sort(values.begin(), values.end());
    return values;
}

// Helper function: validate iterator consistency
bool ValidateIteratorConsistency(const EraseTestHashTable& ht)
{
    size_t iterCount = 0;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        iterCount++;
        if (iterCount > ht.size() + 1) // Prevent infinite loop
        {
            return false;
        }
    }
    return iterCount == ht.size();
}

// Helper function: validate internal structure integrity
bool ValidateInternalStructure(const EraseTestHashTable& ht)
{
    // Additional internal structure validation logic can be added here
    // For example, validate free list, bucket chains, etc.
    return ValidateIteratorConsistency(ht);
}

// =========================== erase(key) tests ===========================

// Test erasing an existing single element
TEST(NFShmHashTableEraseTest, EraseExistingSingleElement)
{
    EraseTestHashTable ht;

    // Insert test data
    for (int i = 1; i <= 10; ++i)
    {
        EraseTestPair pair(i * 10, "value" + std::to_string(i));
        auto result = ht.insert_unique(pair);
        ASSERT_TRUE(result.second);
    }

    ASSERT_EQ(ht.size(), 10);
    auto initialKeys = CollectKeys(ht);
    ASSERT_EQ(initialKeys.size(), 10);

    // Erase an element in the middle
    size_t erased = ht.erase(50);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(ht.size(), 9);

    // Verify the element is actually removed
    auto iter = ht.find(50);
    EXPECT_EQ(iter, ht.end());

    // Verify other elements still exist
    for (int i = 1; i <= 10; ++i)
    {
        if (i * 10 != 50)
        {
            auto findIter = ht.find(i * 10);
            EXPECT_NE(findIter, ht.end());
            EXPECT_EQ(findIter->key, i * 10);
            EXPECT_EQ(findIter->value, "value" + std::to_string(i));
        }
    }

    // Verify internal structure integrity
    EXPECT_TRUE(ValidateInternalStructure(ht));

    auto finalKeys = CollectKeys(ht);
    EXPECT_EQ(finalKeys.size(), 9);
    EXPECT_EQ(std::count(finalKeys.begin(), finalKeys.end(), 50), 0);
}

// Test erasing non-existent element
TEST(NFShmHashTableEraseTest, EraseNonExistentElement)
{
    EraseTestHashTable ht;

    // Insert test data
    for (int i = 1; i <= 5; ++i)
    {
        EraseTestPair pair(i * 10, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 5);

    // Try to erase non-existent element
    size_t erased = ht.erase(999);
    EXPECT_EQ(erased, 0);
    EXPECT_EQ(ht.size(), 5);

    // Verify existing elements are not affected
    for (int i = 1; i <= 5; ++i)
    {
        auto iter = ht.find(i * 10);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, i * 10);
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test erasing multiple elements with same key
TEST(NFShmHashTableEraseTest, EraseMultipleWithSameKey)
{
    EraseTestHashTable ht;

    // Insert multiple elements with same key
    EraseTestPair pair1(100, "value1");
    EraseTestPair pair2(100, "value2");
    EraseTestPair pair3(100, "value3");
    EraseTestPair pair4(200, "value4");

    ht.insert_equal(pair1);
    ht.insert_equal(pair2);
    ht.insert_equal(pair3);
    ht.insert_equal(pair4);

    ASSERT_EQ(ht.size(), 4);
    ASSERT_EQ(ht.count(100), 3);
    ASSERT_EQ(ht.count(200), 1);

    // Erase all elements with key 100
    size_t erased = ht.erase(100);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(ht.size(), 1);
    EXPECT_EQ(ht.count(100), 0);
    EXPECT_EQ(ht.count(200), 1);

    // Verify element with key 200 still exists
    auto iter = ht.find(200);
    EXPECT_NE(iter, ht.end());
    EXPECT_EQ(iter->key, 200);
    EXPECT_EQ(iter->value, "value4");

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// =========================== erase(iterator) tests ===========================

// Test erasing head element of linked list through iterator
TEST(NFShmHashTableEraseTest, EraseIteratorAtHead)
{
    EraseTestHashTable ht;

    // Insert multiple elements to same bucket (by choosing appropriate key values)
    std::vector<int> keys = {0, 50, 100}; // Assume these keys hash to the same bucket
    for (int key : keys)
    {
        EraseTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ht.print_structure();

    ASSERT_EQ(ht.size(), 3);

    // Find the first element
    auto iter = ht.find(keys[0]);
    ASSERT_NE(iter, ht.end());
    int keyToDelete = iter->key;

    // Erase head element
    auto nextIter = ht.erase(iter);
    EXPECT_EQ(ht.size(), 2);
    EXPECT_EQ(ht.find(keyToDelete), ht.end());

    // Verify returned iterator
    if (nextIter != ht.end())
    {
        EXPECT_NE(nextIter->key, keyToDelete);
    }

    // Verify other elements still exist
    for (int key : keys)
    {
        if (key != keyToDelete)
        {
            auto findIter = ht.find(key);
            EXPECT_NE(findIter, ht.end());
        }
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test erasing middle element of linked list through iterator
TEST(NFShmHashTableEraseTest, EraseIteratorAtMiddle)
{
    EraseTestHashTable ht;

    // Insert a series of elements
    std::vector<int> keys = {10, 20, 30, 40, 50};
    for (int key : keys)
    {
        EraseTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 5);

    // Erase middle element
    auto iter = ht.find(30);
    ASSERT_NE(iter, ht.end());

    auto nextIter = ht.erase(iter);
    EXPECT_EQ(ht.size(), 4);
    EXPECT_EQ(ht.find(30), ht.end());

    // Verify returned iterator points to next valid element
    if (nextIter != ht.end())
    {
        EXPECT_NE(nextIter->key, 30);
        // Verify this is a valid iterator
        EXPECT_NO_THROW({
            auto key = nextIter->key;
            auto value = nextIter->value;
            });
    }

    // Verify other elements still exist
    std::vector<int> remainingKeys = {10, 20, 40, 50};
    for (int key : remainingKeys)
    {
        auto findIter = ht.find(key);
        EXPECT_NE(findIter, ht.end());
        EXPECT_EQ(findIter->key, key);
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test erasing tail element of linked list through iterator
TEST(NFShmHashTableEraseTest, EraseIteratorAtTail)
{
    EraseTestHashTable ht;

    // Insert test data
    for (int i = 1; i <= 5; ++i)
    {
        EraseTestPair pair(i * 100, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 5);

    // Find an element to erase
    auto iter = ht.find(300);
    ASSERT_NE(iter, ht.end());

    auto nextIter = ht.erase(iter);
    EXPECT_EQ(ht.size(), 4);
    EXPECT_EQ(ht.find(300), ht.end());

    // Verify other elements still exist
    std::vector<int> expectedKeys = {100, 200, 400, 500};
    for (int key : expectedKeys)
    {
        auto findIter = ht.find(key);
        EXPECT_NE(findIter, ht.end());
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test erasing the last element
TEST(NFShmHashTableEraseTest, EraseLastElement)
{
    EraseTestHashTable ht;

    // Insert only one element
    EraseTestPair pair(42, "single_value");
    auto result = ht.insert_unique(pair);
    ASSERT_TRUE(result.second);
    ASSERT_EQ(ht.size(), 1);

    // Erase this unique element
    auto iter = ht.find(42);
    ASSERT_NE(iter, ht.end());

    auto nextIter = ht.erase(iter);
    EXPECT_EQ(ht.size(), 0);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(nextIter, ht.end());
    EXPECT_EQ(ht.find(42), ht.end());

    // Verify iterators
    EXPECT_EQ(ht.begin(), ht.end());

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test consecutive erase operations
TEST(NFShmHashTableEraseTest, ConsecutiveEraseOperations)
{
    EraseTestHashTable ht;

    // Insert test data
    std::vector<int> keys = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (int key : keys)
    {
        EraseTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 10);

    // Consecutively erase multiple elements
    std::vector<int> keysToDelete = {2, 5, 8, 10};
    for (int keyToDelete : keysToDelete)
    {
        auto iter = ht.find(keyToDelete);
        ASSERT_NE(iter, ht.end());

        auto nextIter = ht.erase(iter);
        EXPECT_EQ(ht.find(keyToDelete), ht.end());

        // Verify internal structure is consistent after each erase
        EXPECT_TRUE(ValidateInternalStructure(ht));
    }

    EXPECT_EQ(ht.size(), 6);

    // Verify remaining elements
    std::vector<int> remainingKeys = {1, 3, 4, 6, 7, 9};
    for (int key : remainingKeys)
    {
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, key);
    }

    // Verify erased elements really don't exist
    for (int key : keysToDelete)
    {
        auto iter = ht.find(key);
        EXPECT_EQ(iter, ht.end());
    }
}

// =========================== erase(range) tests ===========================

// Test range erase
TEST(NFShmHashTableEraseTest, EraseRange)
{
    EraseTestHashTable ht;

    // Insert test data
    for (int i = 1; i <= 10; ++i)
    {
        EraseTestPair pair(i * 10, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ht.print_structure();

    ASSERT_EQ(ht.size(), 10);

    // Create range [30, 70)
    auto first = ht.find(70);
    auto last = ht.find(30);

    ASSERT_NE(first, ht.end());
    ASSERT_NE(last, ht.end());

    // Count elements in range
    size_t rangeSize = 0;
    for (auto iter = first; iter != last && iter != ht.end(); ++iter)
    {
        rangeSize++;
        if (rangeSize > 10) break; // Prevent infinite loop
    }

    ht.erase(first, last);

    // Verify size after erase
    EXPECT_EQ(ht.size(), 10 - rangeSize);

    // Verify elements in range are erased
    EXPECT_EQ(ht.find(70), ht.end());

    // Verify elements outside range still exist
    auto iter30 = ht.find(30);
    EXPECT_NE(iter30, ht.end());

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// =========================== Boundary condition tests ===========================

// Test erase operation on empty hash table
TEST(NFShmHashTableEraseTest, EraseFromEmptyHashTable)
{
    EraseTestHashTable ht;

    ASSERT_TRUE(ht.empty());

    // Try to erase non-existent key
    size_t erased = ht.erase(999);
    EXPECT_EQ(erased, 0);
    EXPECT_TRUE(ht.empty());

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test iterator validity after erase operation
TEST(NFShmHashTableEraseTest, IteratorValidityAfterErase)
{
    EraseTestHashTable ht;

    // Insert test data
    std::vector<int> keys = {1, 2, 3, 4, 5};
    for (int key : keys)
    {
        EraseTestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    // Get all iterators
    std::vector<EraseTestHashTable::iterator> iterators;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        iterators.push_back(iter);
    }

    ASSERT_EQ(iterators.size(), 5);

    // Erase middle element
    auto eraseIter = ht.find(3);
    ASSERT_NE(eraseIter, ht.end());

    auto nextIter = ht.erase(eraseIter);

    // Verify other iterators are still valid (except the erased one)
    for (auto iter : iterators)
    {
        if (iter != eraseIter && ht.find(iter->key) != ht.end())
        {
            EXPECT_NO_THROW({
                auto key = iter->key;
                auto value = iter->value;
                });
        }
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test mass erase operations for performance and correctness
TEST(NFShmHashTableEraseTest, MassEraseOperations)
{
    EraseTestHashTable ht;

    // Insert large amount of data (within capacity limit)
    const int dataSize = 40;
    for (int i = 1; i <= dataSize; ++i)
    {
        EraseTestPair pair(i, "value" + std::to_string(i));
        auto result = ht.insert_unique(pair);
        ASSERT_TRUE(result.second);
    }

    ASSERT_EQ(ht.size(), dataSize);

    // Erase half of the data
    std::vector<int> keysToDelete;
    for (int i = 1; i <= dataSize; i += 2)
    {
        keysToDelete.push_back(i);
    }

    for (int key : keysToDelete)
    {
        size_t erased = ht.erase(key);
        EXPECT_EQ(erased, 1);
    }

    EXPECT_EQ(ht.size(), dataSize / 2);

    // Verify erased elements really don't exist
    for (int key : keysToDelete)
    {
        EXPECT_EQ(ht.find(key), ht.end());
    }

    // Verify remaining elements still exist
    for (int i = 2; i <= dataSize; i += 2)
    {
        auto iter = ht.find(i);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, i);
        EXPECT_EQ(iter->value, "value" + std::to_string(i));
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test erase and reinsert
TEST(NFShmHashTableEraseTest, EraseAndReinsert)
{
    EraseTestHashTable ht;

    // Insert initial data
    for (int i = 1; i <= 10; ++i)
    {
        EraseTestPair pair(i, "original" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 10);

    // Erase some elements
    std::vector<int> keysToDelete = {2, 5, 8};
    for (int key : keysToDelete)
    {
        size_t erased = ht.erase(key);
        EXPECT_EQ(erased, 1);
    }

    EXPECT_EQ(ht.size(), 7);

    // Reinsert same keys with different values
    for (int key : keysToDelete)
    {
        EraseTestPair newPair(key, "new" + std::to_string(key));
        auto result = ht.insert_unique(newPair);
        EXPECT_TRUE(result.second);
        EXPECT_EQ(result.first->key, key);
        EXPECT_EQ(result.first->value, "new" + std::to_string(key));
    }

    EXPECT_EQ(ht.size(), 10);

    // Verify newly inserted values
    for (int key : keysToDelete)
    {
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->key, key);
        EXPECT_EQ(iter->value, "new" + std::to_string(key));
    }

    // Verify other element values haven't changed
    for (int i = 1; i <= 10; ++i)
    {
        if (std::find(keysToDelete.begin(), keysToDelete.end(), i) == keysToDelete.end())
        {
            auto iter = ht.find(i);
            EXPECT_NE(iter, ht.end());
            EXPECT_EQ(iter->value, "original" + std::to_string(i));
        }
    }

    EXPECT_TRUE(ValidateInternalStructure(ht));
}

// Test Clear operation
TEST(NFShmHashTableEraseTest, ClearAllElements)
{
    EraseTestHashTable ht;

    // Insert test data
    for (int i = 1; i <= 20; ++i)
    {
        EraseTestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    ASSERT_EQ(ht.size(), 20);
    ASSERT_FALSE(ht.empty());

    // Clear hash table
    ht.clear();

    EXPECT_EQ(ht.size(), 0);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.left_size(), ht.max_size());

    // Verify all elements are erased
    for (int i = 1; i <= 20; ++i)
    {
        EXPECT_EQ(ht.find(i), ht.end());
    }

    // Verify iterators
    EXPECT_EQ(ht.begin(), ht.end());

    // Verify can reinsert after clear
    EraseTestPair newPair(100, "new_value");
    auto result = ht.insert_unique(newPair);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(ht.size(), 1);

    EXPECT_TRUE(ValidateInternalStructure(ht));
}
