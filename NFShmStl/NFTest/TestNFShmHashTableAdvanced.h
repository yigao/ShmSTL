// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashTableAdvanced.h
//    @Author           :    gaoyi
//    @Date             :    2025/1/27
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashTableAdvanced
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashTable.h"
#include <string>
#include <functional>
#include <chrono>
#include <random>

// Test complex data structure
struct ComplexData
{
    int id;
    std::string name;
    double value;
    std::vector<int> data;

    ComplexData() : id(0), value(0.0)
    {
    }

    ComplexData(int i, const std::string& n, double v)
        : id(i), name(n), value(v)
    {
        data.resize(10);
        //std::iota(data.begin(), data.end(), i);
    }

    bool operator==(const ComplexData& other) const
    {
        return id == other.id && name == other.name &&
            std::abs(value - other.value) < 1e-9 && data == other.data;
    }
};

// Extract key from complex data
struct ComplexExtractKey
{
    int operator()(const ComplexData& data) const
    {
        return data.id;
    }
};

// Hash function that intentionally creates conflicts
struct ConflictHashFunc
{
    size_t operator()(int key) const
    {
        // Map all keys to the same few buckets to create conflicts
        return key % 5;
    }
};

// Normal hash function
struct NormalHashFunc
{
    size_t operator()(int key) const
    {
        return std::hash<int>{}(key);
    }
};

// Key comparison function
struct IntEqualKey
{
    bool operator()(int key1, int key2) const
    {
        return key1 == key2;
    }
};

// Define test complex hash table types
using ComplexHashTable = NFShmHashTable<ComplexData, int, 1000, NormalHashFunc, ComplexExtractKey, IntEqualKey>;
using ConflictHashTable = NFShmHashTable<ComplexData, int, 100, ConflictHashFunc, ComplexExtractKey, IntEqualKey>;

// Performance test
/*TEST(NFShmHashTableAdvancedTest, PerformanceTest)
{
    ComplexHashTable ht;
    const int test_size = 500; // Don't exceed capacity

    // Prepare test data
    std::vector<ComplexData> test_data;
    test_data.reserve(test_size);
    for (int i = 0; i < test_size; ++i)
    {
        test_data.emplace_back(i, "name_" + std::to_string(i), i * 1.5);
    }

    // Test insert performance
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& data : test_data)
    {
        auto result = ht.insert_unique(data);
        EXPECT_TRUE(result.second);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(ht.size(), test_size);
    std::cout << "Insert " << test_size << " elements time: "
        << insert_duration.count() << " microseconds" << std::endl;

    // Test find performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_size; ++i)
    {
        auto iter = ht.find(i);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->id, i);
    }
    end = std::chrono::high_resolution_clock::now();
    auto find_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Find " << test_size << " elements time: "
        << find_duration.count() << " microseconds" << std::endl;

    // Test iteration performance
    start = std::chrono::high_resolution_clock::now();
    int count = 0;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        count++;
    }
    end = std::chrono::high_resolution_clock::now();
    auto iterate_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(count, test_size);
    std::cout << "Iterate " << test_size << " elements time: "
        << iterate_duration.count() << " microseconds" << std::endl;

    // Test erase performance
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < test_size / 2; ++i)
    {
        size_t erased = ht.erase(i);
        EXPECT_EQ(erased, 1);
    }
    end = std::chrono::high_resolution_clock::now();
    auto erase_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(ht.size(), test_size - test_size / 2);
    std::cout << "Erase " << test_size / 2 << " elements time: "
        << erase_duration.count() << " microseconds" << std::endl;
}*/

// Hash collision test
TEST(NFShmHashTableAdvancedTest, HashCollisionTest)
{
    ConflictHashTable ht;
    const int test_size = 50; // 50 elements mapped to 5 buckets, average 10 elements per bucket

    // Insert data, intentionally create conflicts
    for (int i = 0; i < test_size; ++i)
    {
        ComplexData data(i, "conflict_" + std::to_string(i), i * 2.0);
        auto result = ht.insert_unique(data);
        EXPECT_TRUE(result.second);
    }

    EXPECT_EQ(ht.size(), test_size);

    // Verify all data can be found correctly
    for (int i = 0; i < test_size; ++i)
    {
        auto iter = ht.find(i);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->id, i);
        EXPECT_EQ(iter->name, "conflict_" + std::to_string(i));
        EXPECT_DOUBLE_EQ(iter->value, i * 2.0);
    }

    // Check bucket distribution
    std::cout << "Hash collision test - Bucket distribution:" << std::endl;
    size_t max_bucket_size = 0;
    size_t min_bucket_size = SIZE_MAX;
    size_t non_empty_buckets = 0;

    for (size_t i = 0; i < ht.bucket_count(); ++i)
    {
        size_t bucket_size = ht.elems_in_bucket(i);
        if (bucket_size > 0)
        {
            non_empty_buckets++;
            max_bucket_size = std::max(max_bucket_size, bucket_size);
            min_bucket_size = std::min(min_bucket_size, bucket_size);
            //std::cout << "  Bucket " << i << ": " << bucket_size << " elements" << std::endl;
        }
    }

    std::cout << "Non-empty bucket count: " << non_empty_buckets << std::endl;
    std::cout << "Max bucket size: " << max_bucket_size << std::endl;
    std::cout << "Min bucket size: " << min_bucket_size << std::endl;

    // Since we intentionally create conflicts, there should be only 5 non-empty buckets
    EXPECT_EQ(non_empty_buckets, 5);
    EXPECT_GT(max_bucket_size, 1); // Should have conflicts
}

// Random operations test
TEST(NFShmHashTableAdvancedTest, RandomOperationsTest)
{
    ComplexHashTable ht;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> key_dist(1, 1000);
    std::uniform_int_distribution<> op_dist(1, 4); // 1=insert, 2=find, 3=erase, 4=count

    std::set<int> inserted_keys;
    const int num_operations = 1000;

    for (int op = 0; op < num_operations; ++op)
    {
        int key = key_dist(gen);
        int operation = op_dist(gen);

        switch (operation)
        {
            case 1: // insert
            {
                if (ht.size() < ht.max_size() && inserted_keys.find(key) == inserted_keys.end())
                {
                    ComplexData data(key, "random_" + std::to_string(key), key * 1.1);
                    auto result = ht.insert_unique(data);
                    if (result.second)
                    {
                        inserted_keys.insert(key);
                    }
                }
                break;
            }
            case 2: // find
            {
                auto iter = ht.find(key);
                bool should_exist = inserted_keys.find(key) != inserted_keys.end();
                EXPECT_EQ(iter != ht.end(), should_exist);
                if (iter != ht.end())
                {
                    EXPECT_EQ(iter->id, key);
                }
                break;
            }
            case 3: // erase
            {
                size_t erased = ht.erase(key);
                bool should_exist = inserted_keys.find(key) != inserted_keys.end();
                EXPECT_EQ(erased > 0, should_exist);
                if (erased > 0)
                {
                    inserted_keys.erase(key);
                }
                break;
            }
            case 4: // count
            {
                size_t count = ht.count(key);
                bool should_exist = inserted_keys.find(key) != inserted_keys.end();
                EXPECT_EQ(count > 0, should_exist);
                if (should_exist)
                {
                    EXPECT_EQ(count, 1);
                }
                break;
            }
        }
    }

    // Verify final state
    EXPECT_EQ(ht.size(), inserted_keys.size());

    // Verify all keys that should exist can be found
    for (int key : inserted_keys)
    {
        auto iter = ht.find(key);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->id, key);
    }

    std::cout << "Random operations test completed, final size: " << ht.size() << std::endl;
}

// Memory layout and alignment test
TEST(NFShmHashTableAdvancedTest, MemoryLayoutTest)
{
    ComplexHashTable ht;

    // Test memory alignment
    EXPECT_EQ(alignof(ComplexHashTable), alignof(void*));

    // Insert some data
    for (int i = 0; i < 10; ++i)
    {
        ComplexData data(i, "layout_" + std::to_string(i), i * 3.14);
        ht.insert_unique(data);
    }

    // Test node memory layout
    for (int i = 0; i < 10; ++i)
    {
        auto node = ht.GetValidNode(i);
        if (node)
        {
            EXPECT_TRUE(node->m_valid);
            EXPECT_EQ(node->m_self, i);
            EXPECT_EQ(node->m_value.id, i);
        }
    }

    std::cout << "ComplexHashTable size: " << sizeof(ComplexHashTable) << " bytes" << std::endl;
    std::cout << "ComplexData size: " << sizeof(ComplexData) << " bytes" << std::endl;
}

// Exception safety test
TEST(NFShmHashTableAdvancedTest, ExceptionSafetyTest)
{
    ComplexHashTable ht;

    // Fill the hash table
    for (int i = 0; i < 1000; ++i)
    {
        ComplexData data(i, "exception_" + std::to_string(i), i * 2.5);
        auto result = ht.insert_unique(data);
        EXPECT_TRUE(result.second);
    }

    EXPECT_TRUE(ht.full());

    // Try to insert more elements, should fail but not crash
    for (int i = 1000; i < 1010; ++i)
    {
        ComplexData data(i, "overflow_" + std::to_string(i), i * 2.5);
        auto result = ht.insert_unique(data);
        EXPECT_FALSE(result.second);
        EXPECT_EQ(result.first, ht.end());
    }

    // Hash table state should remain consistent
    EXPECT_EQ(ht.size(), 1000);
    EXPECT_TRUE(ht.full());

    // Verify original data is still intact
    for (int i = 0; i < 100; ++i) // Only check first 100 to avoid long test time
    {
        auto iter = ht.find(i);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->id, i);
        EXPECT_EQ(iter->name, "exception_" + std::to_string(i));
    }
}

// Large equal operations test
TEST(NFShmHashTableAdvancedTest, LargeEqualOperationsTest)
{
    ComplexHashTable ht;
    const int base_key = 42;
    const int num_equal_elements = 50;

    // Insert many elements with the same key
    for (int i = 0; i < num_equal_elements; ++i)
    {
        ComplexData data(base_key, "equal_" + std::to_string(i), i * 1.5);
        auto iter = ht.insert_equal(data);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->id, base_key);
    }

    EXPECT_EQ(ht.size(), num_equal_elements);
    EXPECT_EQ(ht.count(base_key), num_equal_elements);

    // Test equal_range
    auto range = ht.equal_range(base_key);
    int count = 0;
    std::set<std::string> found_names;

    for (auto iter = range.first; iter != range.second; ++iter)
    {
        EXPECT_EQ(iter->id, base_key);
        found_names.insert(iter->name);
        count++;
    }

    EXPECT_EQ(count, num_equal_elements);
    EXPECT_EQ(found_names.size(), num_equal_elements);

    // Verify all names are found
    for (int i = 0; i < num_equal_elements; ++i)
    {
        std::string expected_name = "equal_" + std::to_string(i);
        EXPECT_TRUE(found_names.count(expected_name) > 0);
    }

    // Test deleting all elements with the same key
    size_t erased = ht.erase(base_key);
    EXPECT_EQ(erased, num_equal_elements);
    EXPECT_EQ(ht.size(), 0);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.count(base_key), 0);
}

// Iterator stability test
TEST(NFShmHashTableAdvancedTest, IteratorStabilityTest)
{
    ComplexHashTable ht;

    // Insert test data
    std::vector<int> keys;
    for (int i = 1; i <= 20; ++i)
    {
        ComplexData data(i, "stable_" + std::to_string(i), i * 1.8);
        ht.insert_unique(data);
        keys.push_back(i);
    }

    // Collect all iterators
    std::vector<ComplexHashTable::iterator> iterators;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        iterators.push_back(iter);
    }

    EXPECT_EQ(iterators.size(), keys.size());

    // Delete some elements, test if other iterators are still valid
    std::vector<int> keys_to_delete = {5, 10, 15};
    for (int key : keys_to_delete)
    {
        ht.erase(key);
    }

    // Verify iterators for non-deleted elements are still valid
    int valid_count = 0;
    for (auto iter : iterators)
    {
        if (iter.m_curNode && iter.m_curNode->m_valid && iter != ht.end())
        {
            // Check if this iterator points to an element that should still exist
            bool should_exist = std::find(keys_to_delete.begin(), keys_to_delete.end(), iter->id) == keys_to_delete.end();
            if (should_exist)
            {
                // Verify data integrity
                EXPECT_TRUE(iter->name.find("stable_") == 0);
                valid_count++;
            }
        }
    }

    EXPECT_EQ(valid_count + keys_to_delete.size(), keys.size());
    EXPECT_EQ(ht.size(), keys.size() - keys_to_delete.size());
}
