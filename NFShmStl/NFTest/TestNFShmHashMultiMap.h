// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashMultiMap.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashMultiMap
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashMultiMap.h"
#include "NFComm/NFShmStl/NFShmPair.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>

/****************************************************************************
 * NFShmHashMultiMap 单元测试
 ****************************************************************************
 * 
 * 测试目标：
 * 1. 验证与std::unordered_multimap的API兼容性
 * 2. 测试允许重复键的多重映射特性
 * 3. 测试共享内存特有功能（固定容量、CreateInit/ResumeInit等）
 * 4. 验证哈希表特有的性能特征和行为
 * 5. 测试与STL容器的互操作性
 * 6. 验证不存在operator[]的正确性（多重映射语义不明确）
 * 7. 测试错误处理和边界条件
 *****************************************************************************/

// 自定义值类型用于测试
class HashMultiMapTestValue
{
public:
    HashMultiMapTestValue() : id(0), name("default")
    {
        ++constructor_count;
    }

    HashMultiMapTestValue(int i) : id(i), name("value_" + std::to_string(i))
    {
        ++constructor_count;
    }

    HashMultiMapTestValue(int i, const std::string& n) : id(i), name(n)
    {
        ++constructor_count;
    }

    HashMultiMapTestValue(const HashMultiMapTestValue& other) : id(other.id), name(other.name)
    {
        ++constructor_count;
    }

    HashMultiMapTestValue& operator=(const HashMultiMapTestValue& other)
    {
        if (this != &other)
        {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    ~HashMultiMapTestValue()
    {
        ++destructor_count;
    }

    bool operator==(const HashMultiMapTestValue& other) const
    {
        return id == other.id && name == other.name;
    }

    int id;
    std::string name;

    static int constructor_count;
    static int destructor_count;

    static void reset_counters()
    {
        constructor_count = 0;
        destructor_count = 0;
    }
};

int HashMultiMapTestValue::constructor_count = 0;
int HashMultiMapTestValue::destructor_count = 0;

// 自定义哈希函数用于测试
struct CustomMultiMapHasher
{
    std::size_t operator()(const std::string& key) const
    {
        std::size_t hash = 0;
        for (char c : key)
        {
            hash = hash * 31 + c;
        }
        return hash;
    }
};

class NFShmHashMultiMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        HashMultiMapTestValue::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(HashMultiMapTestValue::constructor_count, HashMultiMapTestValue::destructor_count);
    }
};

// ==================== 基本功能测试 ====================

TEST_F(NFShmHashMultiMapTest, BasicOperationsWithIntString)
{
    NFShmHashMultiMap<int, std::string, 16> multiMap;

    // 测试空容器
    EXPECT_TRUE(multiMap.empty());
    EXPECT_EQ(multiMap.size(), 0);
    EXPECT_EQ(multiMap.max_size(), 16);
    EXPECT_FALSE(multiMap.full());
    EXPECT_EQ(multiMap.left_size(), 16);

    // 测试插入（多重映射总是成功插入）
    auto it = multiMap.insert({1, "one"});
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
    EXPECT_EQ(multiMap.size(), 1);
    EXPECT_FALSE(multiMap.empty());
    EXPECT_EQ(multiMap.left_size(), 15);

    // 测试重复键插入（与unordered_map不同，多重映射允许重复键）
    it = multiMap.insert({1, "another one"});
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "another one");
    EXPECT_EQ(multiMap.size(), 2); // 大小增加

    // 再次插入相同键
    multiMap.insert({1, "third one"});
    EXPECT_EQ(multiMap.size(), 3);

    // 插入不同键
    multiMap.insert({2, "two"});
    multiMap.insert({3, "three"});
    EXPECT_EQ(multiMap.size(), 5);

    // 测试查找（返回第一个匹配的元素）
    auto find_it = multiMap.find(1);
    EXPECT_NE(find_it, multiMap.end());
    EXPECT_EQ(find_it->first, 1);
    // 注意：不确定返回哪个"one"，但至少要匹配键

    find_it = multiMap.find(100);
    EXPECT_EQ(find_it, multiMap.end());

    // 测试count（对于多重映射，count可以返回>1）
    EXPECT_EQ(multiMap.count(1), 3); // 键1有3个值
    EXPECT_EQ(multiMap.count(2), 1); // 键2有1个值
    EXPECT_EQ(multiMap.count(100), 0); // 键100不存在

    // 测试equal_range
    auto range = multiMap.equal_range(1);
    size_t count = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->first, 1);
        ++count;
    }
    EXPECT_EQ(count, 3); // 应该有3个元素

    // 测试删除（删除所有匹配的键）
    size_t erased = multiMap.erase(1);
    EXPECT_EQ(erased, 3); // 删除了3个元素
    EXPECT_EQ(multiMap.size(), 2);
    EXPECT_EQ(multiMap.find(1), multiMap.end());

    // 测试迭代器删除
    find_it = multiMap.find(2);
    EXPECT_NE(find_it, multiMap.end());
    auto next_it = multiMap.erase(find_it);
    EXPECT_EQ(multiMap.size(), 1);
    EXPECT_EQ(multiMap.find(2), multiMap.end());
}

// ==================== 多重键测试 ====================

TEST_F(NFShmHashMultiMapTest, MultipleKeysOperations)
{
    NFShmHashMultiMap<std::string, int, 20> multiMap;

    // 插入多个相同键的不同值
    std::string key = "numbers";
    multiMap.insert({key, 1});
    multiMap.insert({key, 2});
    multiMap.insert({key, 3});
    multiMap.insert({key, 4});
    multiMap.insert({key, 5});

    EXPECT_EQ(multiMap.size(), 5);
    EXPECT_EQ(multiMap.count(key), 5);

    // 测试equal_range遍历所有相同键的元素
    auto range = multiMap.equal_range(key);
    std::vector<int> values;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->first, key);
        values.push_back(it->second);
    }
    EXPECT_EQ(values.size(), 5);

    // 验证所有值都存在（注意：顺序可能不确定）
    std::sort(values.begin(), values.end());
    std::vector<int> expected = {1, 2, 3, 4, 5};
    EXPECT_EQ(values, expected);

    // 测试插入其他键
    multiMap.insert({"other", 100});
    multiMap.insert({"other", 200});
    EXPECT_EQ(multiMap.size(), 7);
    EXPECT_EQ(multiMap.count("other"), 2);
    EXPECT_EQ(multiMap.count(key), 5);

    // 测试删除一个键的所有值
    size_t erased = multiMap.erase(key);
    EXPECT_EQ(erased, 5);
    EXPECT_EQ(multiMap.size(), 2);
    EXPECT_EQ(multiMap.count(key), 0);
    EXPECT_EQ(multiMap.count("other"), 2);
}

// ==================== 自定义类型测试 ====================

TEST_F(NFShmHashMultiMapTest, CustomTypeOperations)
{
    NFShmHashMultiMap<std::string, HashMultiMapTestValue, 15> multiMap;

    // 测试自定义类型插入
    HashMultiMapTestValue val1(1, "first");
    HashMultiMapTestValue val2(2, "second");
    HashMultiMapTestValue val3(3, "third");

    auto it = multiMap.insert({"key1", val1});
    EXPECT_EQ(it->first, "key1");
    EXPECT_EQ(it->second.id, 1);
    EXPECT_EQ(it->second.name, "first");

    // 在相同键下插入多个值
    multiMap.insert({"key1", val2});
    multiMap.insert({"key1", val3});
    EXPECT_EQ(multiMap.size(), 3);
    EXPECT_EQ(multiMap.count("key1"), 3);

    // 测试查找自定义类型
    auto find_it = multiMap.find("key1");
    EXPECT_NE(find_it, multiMap.end());
    EXPECT_EQ(find_it->first, "key1");

    // 遍历所有相同键的元素
    auto range = multiMap.equal_range("key1");
    std::vector<int> ids;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        ids.push_back(iter->second.id);
    }
    EXPECT_EQ(ids.size(), 3);

    // 验证所有ID都存在
    std::sort(ids.begin(), ids.end());
    std::vector<int> expected_ids = {1, 2, 3};
    EXPECT_EQ(ids, expected_ids);
}

// ==================== 容量和固定大小测试 ====================

TEST_F(NFShmHashMultiMapTest, CapacityAndFixedSizeOperations)
{
    const size_t MAX_SIZE = 8;
    NFShmHashMultiMap<int, std::string, MAX_SIZE> multiMap;

    // 填充到接近容量
    for (int i = 0; i < MAX_SIZE - 1; ++i)
    {
        auto it = multiMap.insert({i % 3, "value" + std::to_string(i)}); // 使用模运算创建重复键
        EXPECT_EQ(multiMap.size(), i + 1);
        EXPECT_EQ(multiMap.left_size(), MAX_SIZE - i - 1);
        EXPECT_FALSE(multiMap.full());
    }

    // 插入最后一个元素
    auto it = multiMap.insert({100, "last"});
    EXPECT_EQ(multiMap.size(), MAX_SIZE);
    EXPECT_EQ(multiMap.left_size(), 0);
    EXPECT_TRUE(multiMap.full());

    // 尝试插入超出容量的元素（应该失败或者不增加大小）
    auto failed_it = multiMap.insert({200, "overflow"});
    EXPECT_EQ(multiMap.size(), MAX_SIZE);
    EXPECT_TRUE(multiMap.full());

    // 删除一个元素，验证容量恢复
    multiMap.erase(100);
    EXPECT_LT(multiMap.size(), MAX_SIZE);
    EXPECT_GT(multiMap.left_size(), 0);
    EXPECT_FALSE(multiMap.full());

    // 现在可以插入新元素
    it = multiMap.insert({300, "new"});
    EXPECT_EQ(it->first, 300);
    EXPECT_EQ(it->second, "new");
}

// ==================== 迭代器测试 ====================

TEST_F(NFShmHashMultiMapTest, IteratorOperations)
{
    NFShmHashMultiMap<int, std::string, 20> multiMap;

    // 填充测试数据（包含重复键）
    std::vector<std::pair<int, std::string>> test_data = {
        {1, "one_a"}, {1, "one_b"}, {2, "two_a"}, {2, "two_b"},
        {3, "three"}, {4, "four"}, {5, "five"}
    };

    for (const auto& pair : test_data)
    {
        multiMap.insert(pair);
    }
    EXPECT_EQ(multiMap.size(), test_data.size());

    // 测试正向迭代器
    std::vector<std::pair<int, std::string>> iterated_data;
    for (auto it = multiMap.begin(); it != multiMap.end(); ++it)
    {
        iterated_data.push_back({it->first, it->second});
    }
    EXPECT_EQ(iterated_data.size(), test_data.size());

    // 验证所有元素都能通过迭代器访问到
    for (const auto& pair : test_data)
    {
        auto found = std::find_if(iterated_data.begin(), iterated_data.end(),
                                  [&pair](const auto& p) { return p.first == pair.first && p.second == pair.second; });
        EXPECT_NE(found, iterated_data.end());
    }

    // 测试const迭代器
    const auto& const_map = multiMap;
    std::vector<std::pair<int, std::string>> const_iterated_data;
    for (auto it = const_map.begin(); it != const_map.end(); ++it)
    {
        const_iterated_data.push_back({it->first, it->second});
    }
    EXPECT_EQ(const_iterated_data.size(), test_data.size());

    // 测试cbegin/cend
    std::vector<std::pair<int, std::string>> c_iterated_data;
    for (auto it = multiMap.cbegin(); it != multiMap.cend(); ++it)
    {
        c_iterated_data.push_back({it->first, it->second});
    }
    EXPECT_EQ(c_iterated_data.size(), test_data.size());

    // 测试范围for循环
    std::vector<std::pair<int, std::string>> range_data;
    for (const auto& pair : multiMap)
    {
        range_data.push_back({pair.first, pair.second});
    }
    EXPECT_EQ(range_data.size(), test_data.size());
}

// ==================== STL兼容性测试 ====================

TEST_F(NFShmHashMultiMapTest, STLCompatibility)
{
    // 从std::unordered_multimap构造
    std::unordered_multimap<int, std::string> stdMultiMap = {
        {1, "one"}, {1, "uno"}, {2, "two"}, {2, "dos"}, {3, "three"}
    };

    NFShmHashMultiMap<int, std::string, 15> multiMap(stdMultiMap);
    EXPECT_EQ(multiMap.size(), 5);

    // 验证所有元素都存在
    for (const auto& pair : stdMultiMap)
    {
        auto range = multiMap.equal_range(pair.first);
        bool found = false;
        for (auto it = range.first; it != range.second; ++it)
        {
            if (it->second == pair.second)
            {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    // 从std::multimap构造
    std::multimap<int, std::string> orderedMultiMap = {
        {10, "ten"}, {10, "diez"}, {20, "twenty"}
    };

    NFShmHashMultiMap<int, std::string, 10> multiMap2(orderedMultiMap);
    EXPECT_EQ(multiMap2.size(), 3);

    // 测试赋值操作
    NFShmHashMultiMap<int, std::string, 20> multiMap3;
    multiMap3 = stdMultiMap;
    EXPECT_EQ(multiMap3.size(), 5);

    multiMap3 = orderedMultiMap;
    EXPECT_EQ(multiMap3.size(), 3);

    // 测试初始化列表
    NFShmHashMultiMap<int, std::string, 12> multiMap4{
        {100, "hundred"}, {100, "century"}, {200, "two hundred"}
    };
    EXPECT_EQ(multiMap4.size(), 3);
    EXPECT_EQ(multiMap4.count(100), 2);
    EXPECT_EQ(multiMap4.count(200), 1);

    // 测试初始化列表赋值
    multiMap4 = {{300, "three hundred"}, {300, "another"}, {400, "four hundred"}};
    EXPECT_EQ(multiMap4.size(), 3);
    EXPECT_EQ(multiMap4.count(300), 2);
}

// ==================== 范围插入测试 ====================

TEST_F(NFShmHashMultiMapTest, RangeInsertOperations)
{
    NFShmHashMultiMap<int, std::string, 25> multiMap;

    // 准备测试数据（包含重复键）
    std::vector<std::pair<int, std::string>> data = {
        {1, "one"}, {1, "uno"}, {2, "two"}, {2, "dos"},
        {3, "three"}, {1, "eins"} // 更多重复键
    };

    // 测试迭代器范围插入
    multiMap.insert(data.begin(), data.end());
    EXPECT_EQ(multiMap.size(), 6); // 所有元素都应该被插入

    // 验证count
    EXPECT_EQ(multiMap.count(1), 3); // 键1有3个值
    EXPECT_EQ(multiMap.count(2), 2); // 键2有2个值
    EXPECT_EQ(multiMap.count(3), 1); // 键3有1个值

    // 测试数组范围插入
    std::pair<int, std::string> array_data[] = {
        {10, "ten"}, {10, "diez"}, {20, "twenty"}
    };

    multiMap.insert(array_data, array_data + 3);
    EXPECT_EQ(multiMap.size(), 9);
    EXPECT_EQ(multiMap.count(10), 2);
    EXPECT_EQ(multiMap.count(20), 1);

    // 测试初始化列表插入
    multiMap.insert({{100, "hundred"}, {100, "century"}});
    EXPECT_EQ(multiMap.size(), 11);
    EXPECT_EQ(multiMap.count(100), 2);
}

// ==================== 删除操作测试 ====================

TEST_F(NFShmHashMultiMapTest, EraseOperations)
{
    NFShmHashMultiMap<int, std::string, 20> multiMap;

    // 填充测试数据
    for (int i = 1; i <= 5; ++i)
    {
        multiMap.insert({i, "value" + std::to_string(i) + "_a"});
        multiMap.insert({i, "value" + std::to_string(i) + "_b"});
    }
    EXPECT_EQ(multiMap.size(), 10);

    // 测试按键删除（删除所有匹配的键值对）
    size_t erased = multiMap.erase(1);
    EXPECT_EQ(erased, 2); // 删除了2个元素
    EXPECT_EQ(multiMap.size(), 8);
    EXPECT_EQ(multiMap.find(1), multiMap.end());

    // 测试删除不存在的键
    erased = multiMap.erase(100);
    EXPECT_EQ(erased, 0);
    EXPECT_EQ(multiMap.size(), 8);

    // 测试迭代器删除（只删除单个元素）
    auto it = multiMap.find(2);
    EXPECT_NE(it, multiMap.end());
    auto next_it = multiMap.erase(it);
    EXPECT_EQ(multiMap.size(), 7);
    EXPECT_EQ(multiMap.count(2), 1); // 还剩1个键为2的元素

    // 测试范围删除
    auto range = multiMap.equal_range(3);
    if (range.first != range.second)
    {
        size_t old_size = multiMap.size();
        auto result_it = multiMap.erase(range.first, range.second);
        EXPECT_LT(multiMap.size(), old_size);
        EXPECT_EQ(multiMap.count(3), 0);
    }

    // 测试清空
    multiMap.clear();
    EXPECT_TRUE(multiMap.empty());
    EXPECT_EQ(multiMap.size(), 0);
}

// ==================== 自定义哈希函数测试 ====================

TEST_F(NFShmHashMultiMapTest, CustomHashFunction)
{
    NFShmHashMultiMap<std::string, int, 15, CustomMultiMapHasher> multiMap;

    // 测试自定义哈希函数
    multiMap.insert({"hello", 1});
    multiMap.insert({"hello", 2});
    multiMap.insert({"world", 3});
    multiMap.insert({"world", 4});

    EXPECT_EQ(multiMap.size(), 4);
    EXPECT_EQ(multiMap.count("hello"), 2);
    EXPECT_EQ(multiMap.count("world"), 2);

    // 验证哈希函数正常工作
    auto range = multiMap.equal_range("hello");
    std::vector<int> values;
    for (auto it = range.first; it != range.second; ++it)
    {
        values.push_back(it->second);
    }
    EXPECT_EQ(values.size(), 2);
    std::sort(values.begin(), values.end());
    std::vector<int> expected = {1, 2};
    EXPECT_EQ(values, expected);
}

// ==================== 性能和压力测试 ====================

TEST_F(NFShmHashMultiMapTest, PerformanceAndStressTest)
{
    const size_t LARGE_SIZE = 1000;
    NFShmHashMultiMap<int, int, LARGE_SIZE> multiMap;

    // 大量插入测试（创建重复键）
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        multiMap.insert({i % 100, i}); // 创建重复键
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(multiMap.size(), LARGE_SIZE);
    EXPECT_TRUE(multiMap.full());

    // 验证count功能
    for (int key = 0; key < 100; ++key)
    {
        EXPECT_EQ(multiMap.count(key), 10); // 每个键应该有10个值
    }

    // 大量查找测试
    start = std::chrono::high_resolution_clock::now();

    for (int key = 0; key < 100; ++key)
    {
        auto range = multiMap.equal_range(key);
        size_t count = 0;
        for (auto it = range.first; it != range.second; ++it)
        {
            ++count;
        }
        EXPECT_EQ(count, 10);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 大量删除测试
    start = std::chrono::high_resolution_clock::now();

    for (int key = 0; key < 50; ++key)
    {
        multiMap.erase(key);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(multiMap.size(), LARGE_SIZE / 2);
}

// ==================== 共享内存特有功能测试 ====================

TEST_F(NFShmHashMultiMapTest, SharedMemorySpecificFeatures)
{
    NFShmHashMultiMap<int, std::string, 12> multiMap;

    // 测试CreateInit和ResumeInit
    EXPECT_EQ(multiMap.CreateInit(), 0);
    EXPECT_EQ(multiMap.ResumeInit(), 0);

    // 测试Init方法
    multiMap.insert({1, "test"});
    multiMap.insert({1, "another"});
    EXPECT_EQ(multiMap.size(), 2);

    multiMap.Init();
    EXPECT_EQ(multiMap.size(), 0);
    EXPECT_TRUE(multiMap.empty());

    // 测试桶接口
    EXPECT_EQ(multiMap.bucket_count(), 12);
    EXPECT_EQ(multiMap.max_bucket_count(), 12);

    // 填充一些数据测试桶
    for (int i = 0; i < 6; ++i)
    {
        multiMap.insert({i % 3, "value" + std::to_string(i)});
    }

    // 测试桶中元素数量
    size_t total_elements = 0;
    for (size_t i = 0; i < multiMap.bucket_count(); ++i)
    {
        total_elements += multiMap.elems_in_bucket(i);
    }
    EXPECT_EQ(total_elements, multiMap.size());

    // 测试resize提示（实际不改变大小）
    size_t old_bucket_count = multiMap.bucket_count();
    multiMap.resize(20);
    EXPECT_EQ(multiMap.bucket_count(), old_bucket_count);
}

// ==================== 比较操作符测试 ====================

TEST_F(NFShmHashMultiMapTest, ComparisonOperators)
{
    NFShmHashMultiMap<int, std::string, 15> multiMap1;
    NFShmHashMultiMap<int, std::string, 15> multiMap2;

    // 空容器比较
    EXPECT_TRUE(multiMap1 == multiMap2);

    // 添加相同元素（包括重复键）
    multiMap1.insert({1, "one"});
    multiMap1.insert({1, "uno"});
    multiMap1.insert({2, "two"});

    multiMap2.insert({1, "one"});
    multiMap2.insert({1, "uno"});
    multiMap2.insert({2, "two"});

    EXPECT_TRUE(multiMap1 == multiMap2);

    // 添加不同元素
    multiMap2.insert({3, "three"});
    EXPECT_FALSE(multiMap1 == multiMap2);

    // 相同键不同值
    multiMap1.insert({3, "THREE"});
    EXPECT_FALSE(multiMap1 == multiMap2);
}

// ==================== 交换操作测试 ====================

TEST_F(NFShmHashMultiMapTest, SwapOperations)
{
    NFShmHashMultiMap<int, std::string, 15> multiMap1;
    NFShmHashMultiMap<int, std::string, 15> multiMap2;

    // 准备测试数据
    multiMap1.insert({1, "one"});
    multiMap1.insert({1, "uno"});

    multiMap2.insert({10, "ten"});
    multiMap2.insert({10, "diez"});
    multiMap2.insert({20, "twenty"});

    size_t size1 = multiMap1.size();
    size_t size2 = multiMap2.size();

    // 测试成员函数swap
    multiMap1.swap(multiMap2);

    EXPECT_EQ(multiMap1.size(), size2);
    EXPECT_EQ(multiMap2.size(), size1);

    EXPECT_EQ(multiMap1.count(10), 2);
    EXPECT_EQ(multiMap1.count(20), 1);
    EXPECT_EQ(multiMap1.count(1), 0);

    EXPECT_EQ(multiMap2.count(1), 2);
    EXPECT_EQ(multiMap2.count(10), 0);
}

// ==================== emplace功能测试 ====================

TEST_F(NFShmHashMultiMapTest, EmplaceOperations)
{
    NFShmHashMultiMap<std::string, HashMultiMapTestValue, 12> multiMap;

    // 测试emplace（多重映射总是成功）
    auto it = multiMap.emplace("key1", HashMultiMapTestValue(1, "first"));
    EXPECT_EQ(it->first, "key1");
    EXPECT_EQ(it->second.id, 1);
    EXPECT_EQ(it->second.name, "first");

    // 测试重复键的emplace（应该成功，因为是multimap）
    it = multiMap.emplace("key1", HashMultiMapTestValue(2, "second"));
    EXPECT_EQ(it->first, "key1");
    EXPECT_EQ(it->second.id, 2);
    EXPECT_EQ(it->second.name, "second");

    // 测试emplace_hint
    auto hint_it = multiMap.emplace_hint(multiMap.end(), "key2", HashMultiMapTestValue(3, "third"));
    EXPECT_EQ(hint_it->first, "key2");
    EXPECT_EQ(hint_it->second.id, 3);
    EXPECT_EQ(hint_it->second.name, "third");

    EXPECT_EQ(multiMap.size(), 3);
    EXPECT_EQ(multiMap.count("key1"), 2);
    EXPECT_EQ(multiMap.count("key2"), 1);
}

// ==================== 无operator[]测试 ====================

TEST_F(NFShmHashMultiMapTest, NoOperatorSquareBrackets)
{
    // 验证NFShmHashMultiMap没有operator[]
    // 这是因为对于多重映射，operator[]的语义不明确
    // （一个键可能对应多个值，返回哪个值呢？）

    NFShmHashMultiMap<int, std::string, 10> multiMap;
    multiMap.insert({1, "one"});
    multiMap.insert({1, "uno"});

    // 以下代码应该编译失败（如果取消注释）：
    // std::string value = multiMap[1]; // 应该编译错误

    // 正确的访问方式是使用find或equal_range
    auto it = multiMap.find(1);
    EXPECT_NE(it, multiMap.end());
    EXPECT_EQ(it->first, 1);

    auto range = multiMap.equal_range(1);
    size_t count = 0;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        EXPECT_EQ(iter->first, 1);
        ++count;
    }
    EXPECT_EQ(count, 2);
}
