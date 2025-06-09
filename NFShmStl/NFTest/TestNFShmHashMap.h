// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashMap.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashMap
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmPair.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>

/****************************************************************************
 * NFShmHashMap 单元测试
 ****************************************************************************
 * 
 * 测试目标：
 * 1. 验证与std::unordered_map的API兼容性
 * 2. 测试共享内存特有功能（固定容量、CreateInit/ResumeInit等）
 * 3. 验证哈希表特有的性能特征和行为
 * 4. 测试与STL容器的互操作性
 * 5. 验证错误处理和边界条件
 *****************************************************************************/

// 自定义值类型用于测试
class HashTestValue
{
public:
    HashTestValue() : id(0), name("default")
    {
        ++constructor_count;
    }

    HashTestValue(int i) : id(i), name("value_" + std::to_string(i))
    {
        ++constructor_count;
    }

    HashTestValue(int i, const std::string& n) : id(i), name(n)
    {
        ++constructor_count;
    }

    HashTestValue(const HashTestValue& other) : id(other.id), name(other.name)
    {
        ++constructor_count;
    }

    HashTestValue& operator=(const HashTestValue& other)
    {
        if (this != &other)
        {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    ~HashTestValue()
    {
        ++destructor_count;
    }

    bool operator==(const HashTestValue& other) const
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

int HashTestValue::constructor_count = 0;
int HashTestValue::destructor_count = 0;

// 自定义哈希函数用于测试
struct CustomHasher
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

class NFShmHashMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        HashTestValue::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(HashTestValue::constructor_count, HashTestValue::destructor_count);
    }
};

// ==================== 基本功能测试 ====================

TEST_F(NFShmHashMapTest, BasicOperationsWithIntString)
{
    NFShmHashMap<int, std::string, 16> hashMap;

    // 测试空容器
    EXPECT_TRUE(hashMap.empty());
    EXPECT_EQ(hashMap.size(), 0);
    EXPECT_EQ(hashMap.max_size(), 16);
    EXPECT_FALSE(hashMap.full());
    EXPECT_EQ(hashMap.left_size(), 16);

    // 测试插入
    auto result = hashMap.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
    EXPECT_EQ(hashMap.size(), 1);
    EXPECT_FALSE(hashMap.empty());
    EXPECT_EQ(hashMap.left_size(), 15);

    // 测试重复键插入（哈希map保证键唯一性）
    result = hashMap.insert({1, "another one"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second, "one"); // 保持原值
    EXPECT_EQ(hashMap.size(), 1);

    // 测试operator[]插入和访问
    hashMap[2] = "two";
    EXPECT_EQ(hashMap.size(), 2);
    EXPECT_EQ(hashMap[2], "two");

    // 测试operator[]修改
    hashMap[3] = "three";
    EXPECT_EQ(hashMap[3], "three");
    hashMap[3] = "modified three";
    EXPECT_EQ(hashMap[3], "modified three");

    // 测试查找
    auto it = hashMap.find(1);
    EXPECT_NE(it, hashMap.end());
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");

    it = hashMap.find(100);
    EXPECT_EQ(it, hashMap.end());

    // 测试count（对于哈希map，count只能返回0或1）
    EXPECT_EQ(hashMap.count(1), 1);
    EXPECT_EQ(hashMap.count(100), 0);

    // 测试at方法
    EXPECT_EQ(hashMap.at(2), "two");
    EXPECT_NO_THROW(hashMap.at(100));

    // 测试equal_range
    auto range = hashMap.equal_range(2);
    EXPECT_NE(range.first, hashMap.end());
    EXPECT_EQ(range.first->first, 2);
    EXPECT_EQ(std::distance(range.first, range.second), 1);

    // 测试删除
    size_t erased = hashMap.erase(1);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(hashMap.size(), 2);
    EXPECT_EQ(hashMap.find(1), hashMap.end());

    // 测试迭代器删除
    it = hashMap.find(2);
    EXPECT_NE(it, hashMap.end());
    auto next_it = hashMap.erase(it);
    EXPECT_EQ(hashMap.size(), 1);
    EXPECT_EQ(hashMap.find(2), hashMap.end());
}

// ==================== 自定义类型测试 ====================

TEST_F(NFShmHashMapTest, CustomTypeOperations)
{
    NFShmHashMap<std::string, HashTestValue, 12> hashMap;

    // 测试自定义类型插入
    HashTestValue val1(1, "first");
    HashTestValue val2(2, "second");
    HashTestValue val3(3, "third");

    auto result = hashMap.insert({"key1", val1});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, "key1");
    EXPECT_EQ(result.first->second.id, 1);
    EXPECT_EQ(result.first->second.name, "first");

    hashMap["key2"] = val2;
    hashMap["key3"] = val3;
    EXPECT_EQ(hashMap.size(), 3);

    // 测试自定义类型访问
    EXPECT_EQ(hashMap["key1"].id, 1);
    EXPECT_EQ(hashMap["key1"].name, "first");
    EXPECT_EQ(hashMap.at("key2").id, 2);

    // 测试查找自定义类型
    auto it = hashMap.find("key3");
    EXPECT_NE(it, hashMap.end());
    EXPECT_EQ(it->second.id, 3);
    EXPECT_EQ(it->second.name, "third");

    // 测试修改自定义类型
    hashMap["key1"].id = 100;
    EXPECT_EQ(hashMap["key1"].id, 100);
}

// ==================== 容量和固定大小测试 ====================

TEST_F(NFShmHashMapTest, CapacityAndFixedSizeOperations)
{
    const size_t MAX_SIZE = 8;
    NFShmHashMap<int, std::string, MAX_SIZE> hashMap;

    // 填充到接近容量
    for (int i = 0; i < MAX_SIZE - 1; ++i)
    {
        auto result = hashMap.insert({i, "value" + std::to_string(i)});
        EXPECT_TRUE(result.second);
        EXPECT_EQ(hashMap.size(), i + 1);
        EXPECT_EQ(hashMap.left_size(), MAX_SIZE - i - 1);
        EXPECT_FALSE(hashMap.full());
    }

    // 插入最后一个元素
    auto result = hashMap.insert({MAX_SIZE - 1, "last"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(hashMap.size(), MAX_SIZE);
    EXPECT_EQ(hashMap.left_size(), 0);
    EXPECT_TRUE(hashMap.full());

    // 尝试插入超出容量的元素
    result = hashMap.insert({MAX_SIZE, "overflow"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(hashMap.size(), MAX_SIZE);
    EXPECT_TRUE(hashMap.full());

    // 测试operator[]在满容量时的行为
    std::string& ref = hashMap[MAX_SIZE + 10];
    EXPECT_EQ(hashMap.size(), MAX_SIZE); // 不应该增加

    // 删除一个元素，验证容量恢复
    hashMap.erase(0);
    EXPECT_EQ(hashMap.size(), MAX_SIZE - 1);
    EXPECT_EQ(hashMap.left_size(), 1);
    EXPECT_FALSE(hashMap.full());

    // 现在可以插入新元素
    result = hashMap.insert({MAX_SIZE, "new"});
    EXPECT_TRUE(result.second);
    EXPECT_TRUE(hashMap.full());
}

// ==================== 迭代器测试 ====================

TEST_F(NFShmHashMapTest, IteratorOperations)
{
    NFShmHashMap<int, std::string, 10> hashMap;

    // 填充测试数据
    std::vector<std::pair<int, std::string>> test_data = {
        {1, "one"}, {3, "three"}, {5, "five"}, {7, "seven"}, {9, "nine"}
    };

    for (const auto& pair : test_data)
    {
        hashMap.insert(pair);
    }

    // 测试正向迭代器
    std::vector<std::pair<int, std::string>> iterated_data;
    for (auto it = hashMap.begin(); it != hashMap.end(); ++it)
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
    const auto& const_map = hashMap;
    std::vector<std::pair<int, std::string>> const_iterated_data;
    for (auto it = const_map.begin(); it != const_map.end(); ++it)
    {
        const_iterated_data.push_back({it->first, it->second});
    }
    EXPECT_EQ(const_iterated_data.size(), test_data.size());

    // 测试cbegin/cend
    std::vector<std::pair<int, std::string>> c_iterated_data;
    for (auto it = hashMap.cbegin(); it != hashMap.cend(); ++it)
    {
        c_iterated_data.push_back({it->first, it->second});
    }
    EXPECT_EQ(c_iterated_data.size(), test_data.size());

    // 测试范围for循环
    std::vector<std::pair<int, std::string>> range_data;
    for (const auto& pair : hashMap)
    {
        range_data.push_back({pair.first, pair.second});
    }
    EXPECT_EQ(range_data.size(), test_data.size());
}

// ==================== STL兼容性测试 ====================

TEST_F(NFShmHashMapTest, STLCompatibility)
{
    // 从std::unordered_map构造
    std::unordered_map<int, std::string> stdMap = {
        {1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}
    };

    NFShmHashMap<int, std::string, 10> hashMap(stdMap);
    EXPECT_EQ(hashMap.size(), 4);

    for (const auto& pair : stdMap)
    {
        auto it = hashMap.find(pair.first);
        EXPECT_NE(it, hashMap.end());
        EXPECT_EQ(it->second, pair.second);
    }

    // 从std::map构造
    std::map<int, std::string> orderedMap = {
        {10, "ten"}, {20, "twenty"}, {30, "thirty"}
    };

    NFShmHashMap<int, std::string, 8> hashMap2(orderedMap);
    EXPECT_EQ(hashMap2.size(), 3);

    for (const auto& pair : orderedMap)
    {
        auto it = hashMap2.find(pair.first);
        EXPECT_NE(it, hashMap2.end());
        EXPECT_EQ(it->second, pair.second);
    }

    // 测试赋值操作
    NFShmHashMap<int, std::string, 15> hashMap3;
    hashMap3 = stdMap;
    EXPECT_EQ(hashMap3.size(), 4);

    hashMap3 = orderedMap;
    EXPECT_EQ(hashMap3.size(), 3);

    // 测试初始化列表
    NFShmHashMap<int, std::string, 12> hashMap4{{100, "hundred"}, {200, "two hundred"}};
    EXPECT_EQ(hashMap4.size(), 2);
    EXPECT_EQ(hashMap4[100], "hundred");
    EXPECT_EQ(hashMap4[200], "two hundred");

    // 测试初始化列表赋值
    hashMap4 = {{300, "three hundred"}, {400, "four hundred"}, {500, "five hundred"}};
    EXPECT_EQ(hashMap4.size(), 3);
    EXPECT_EQ(hashMap4[300], "three hundred");
}

// ==================== 范围插入测试 ====================

TEST_F(NFShmHashMapTest, RangeInsertOperations)
{
    NFShmHashMap<int, std::string, 20> hashMap;

    // 准备测试数据
    std::vector<std::pair<int, std::string>> data = {
        {1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}, {5, "five"}
    };

    // 测试迭代器范围插入
    hashMap.insert(data.begin(), data.end());
    EXPECT_EQ(hashMap.size(), 5);

    for (const auto& pair : data)
    {
        auto it = hashMap.find(pair.first);
        EXPECT_NE(it, hashMap.end());
        EXPECT_EQ(it->second, pair.second);
    }

    // 测试数组范围插入
    std::pair<int, std::string> array_data[] = {
        {10, "ten"}, {20, "twenty"}, {30, "thirty"}
    };

    hashMap.insert(array_data, array_data + 3);
    EXPECT_EQ(hashMap.size(), 8);

    for (int i = 0; i < 3; ++i)
    {
        auto it = hashMap.find(array_data[i].first);
        EXPECT_NE(it, hashMap.end());
        EXPECT_EQ(it->second, array_data[i].second);
    }

    // 测试初始化列表插入
    hashMap.insert({{100, "hundred"}, {200, "two hundred"}});
    EXPECT_EQ(hashMap.size(), 10);
    EXPECT_EQ(hashMap[100], "hundred");
    EXPECT_EQ(hashMap[200], "two hundred");
}

// ==================== 删除操作测试 ====================

TEST_F(NFShmHashMapTest, EraseOperations)
{
    NFShmHashMap<int, std::string, 15> hashMap;

    // 填充测试数据
    for (int i = 1; i <= 10; ++i)
    {
        hashMap.insert({i, "value" + std::to_string(i)});
    }
    EXPECT_EQ(hashMap.size(), 10);

    // 测试按键删除
    size_t erased = hashMap.erase(1);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(hashMap.size(), 9);
    EXPECT_EQ(hashMap.find(1), hashMap.end());

    // 测试删除不存在的键
    erased = hashMap.erase(100);
    EXPECT_EQ(erased, 0);
    EXPECT_EQ(hashMap.size(), 9);

    // 测试迭代器删除
    auto it = hashMap.find(2);
    EXPECT_NE(it, hashMap.end());
    auto next_it = hashMap.erase(it);
    EXPECT_EQ(hashMap.size(), 8);
    EXPECT_EQ(hashMap.find(2), hashMap.end());

    // 测试范围删除
    auto first = hashMap.find(3);
    auto last = hashMap.find(6);
    if (first != hashMap.end() && last != hashMap.end())
    {
        ++last; // 范围删除是[first, last)
        size_t old_size = hashMap.size();
        auto result_it = hashMap.erase(first, last);
        EXPECT_EQ(hashMap.size(), old_size);
    }

    // 测试清空
    hashMap.clear();
    EXPECT_TRUE(hashMap.empty());
    EXPECT_EQ(hashMap.size(), 0);
}

// ==================== 自定义哈希函数测试 ====================

TEST_F(NFShmHashMapTest, CustomHashFunction)
{
    NFShmHashMap<std::string, int, 12, CustomHasher> hashMap;

    // 测试自定义哈希函数
    hashMap.insert({"hello", 1});
    hashMap.insert({"world", 2});
    hashMap.insert({"test", 3});

    EXPECT_EQ(hashMap.size(), 3);
    EXPECT_EQ(hashMap["hello"], 1);
    EXPECT_EQ(hashMap["world"], 2);
    EXPECT_EQ(hashMap["test"], 3);

    // 验证哈希函数正常工作
    auto it = hashMap.find("hello");
    EXPECT_NE(it, hashMap.end());
    EXPECT_EQ(it->second, 1);
}

// ==================== 性能和压力测试 ====================

TEST_F(NFShmHashMapTest, PerformanceAndStressTest)
{
    const size_t LARGE_SIZE = 1000;
    NFShmHashMap<int, int, LARGE_SIZE> hashMap;

    // 大量插入测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        hashMap.insert({i, i * 2});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(hashMap.size(), LARGE_SIZE);
    EXPECT_TRUE(hashMap.full());

    // 验证所有元素都存在
    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        auto it = hashMap.find(i);
        EXPECT_NE(it, hashMap.end());
        EXPECT_EQ(it->second, i * 2);
    }

    // 大量查找测试
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        EXPECT_EQ(hashMap.count(i), 1);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 大量删除测试
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE / 2; ++i)
    {
        hashMap.erase(i);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(hashMap.size(), LARGE_SIZE / 2);
}

// ==================== 共享内存特有功能测试 ====================

TEST_F(NFShmHashMapTest, SharedMemorySpecificFeatures)
{
    NFShmHashMap<int, std::string, 10> hashMap;

    // 测试CreateInit和ResumeInit
    EXPECT_EQ(hashMap.CreateInit(), 0);
    EXPECT_EQ(hashMap.ResumeInit(), 0);

    // 测试Init方法
    hashMap.insert({1, "test"});
    EXPECT_EQ(hashMap.size(), 1);

    hashMap.Init();
    EXPECT_EQ(hashMap.size(), 0);
    EXPECT_TRUE(hashMap.empty());

    // 测试桶接口
    EXPECT_EQ(hashMap.bucket_count(), 10);
    EXPECT_EQ(hashMap.max_bucket_count(), 10);

    // 填充一些数据测试桶
    for (int i = 0; i < 5; ++i)
    {
        hashMap.insert({i, "value" + std::to_string(i)});
    }

    // 测试桶中元素数量
    size_t total_elements = 0;
    for (size_t i = 0; i < hashMap.bucket_count(); ++i)
    {
        total_elements += hashMap.elems_in_bucket(i);
    }
    EXPECT_EQ(total_elements, hashMap.size());

    // 测试resize提示（实际不改变大小）
    size_t old_bucket_count = hashMap.bucket_count();
    hashMap.resize(20);
    EXPECT_EQ(hashMap.bucket_count(), old_bucket_count);
}

// ==================== 比较操作符测试 ====================

TEST_F(NFShmHashMapTest, ComparisonOperators)
{
    NFShmHashMap<int, std::string, 10> hashMap1;
    NFShmHashMap<int, std::string, 10> hashMap2;

    // 空容器比较
    EXPECT_TRUE(hashMap1 == hashMap2);

    // 添加相同元素
    hashMap1.insert({1, "one"});
    hashMap1.insert({2, "two"});

    hashMap2.insert({1, "one"});
    hashMap2.insert({2, "two"});

    EXPECT_TRUE(hashMap1 == hashMap2);

    // 添加不同元素
    hashMap2.insert({3, "three"});
    EXPECT_FALSE(hashMap1 == hashMap2);

    // 相同键不同值
    hashMap1.insert({3, "THREE"});
    EXPECT_FALSE(hashMap1 == hashMap2);
}

// ==================== 交换操作测试 ====================

TEST_F(NFShmHashMapTest, SwapOperations)
{
    NFShmHashMap<int, std::string, 10> hashMap1;
    NFShmHashMap<int, std::string, 10> hashMap2;

    // 准备测试数据
    hashMap1.insert({1, "one"});
    hashMap1.insert({2, "two"});

    hashMap2.insert({10, "ten"});
    hashMap2.insert({20, "twenty"});
    hashMap2.insert({30, "thirty"});

    size_t size1 = hashMap1.size();
    size_t size2 = hashMap2.size();

    // 测试成员函数swap
    hashMap1.swap(hashMap2);

    EXPECT_EQ(hashMap1.size(), size2);
    EXPECT_EQ(hashMap2.size(), size1);

    EXPECT_EQ(hashMap1[10], "ten");
    EXPECT_EQ(hashMap1[20], "twenty");
    EXPECT_EQ(hashMap1[30], "thirty");

    EXPECT_EQ(hashMap2[1], "one");
    EXPECT_EQ(hashMap2[2], "two");

    // 测试全局swap函数
    std::swap(hashMap1, hashMap2);

    EXPECT_EQ(hashMap1.size(), size1);
    EXPECT_EQ(hashMap2.size(), size2);

    EXPECT_EQ(hashMap1[1], "one");
    EXPECT_EQ(hashMap1[2], "two");

    EXPECT_EQ(hashMap2[10], "ten");
    EXPECT_EQ(hashMap2[20], "twenty");
    EXPECT_EQ(hashMap2[30], "thirty");
}

// ==================== emplace功能测试 ====================

TEST_F(NFShmHashMapTest, EmplaceOperations)
{
    NFShmHashMap<std::string, HashTestValue, 10> hashMap;

    // 测试emplace
    auto result = hashMap.emplace("key1", HashTestValue(1, "first"));
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, "key1");
    EXPECT_EQ(result.first->second.id, 1);
    EXPECT_EQ(result.first->second.name, "first");

    // 测试重复键的emplace
    result = hashMap.emplace("key1", HashTestValue(2, "second"));
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second.id, 1); // 保持原值

    // 测试emplace_hint
    auto it = hashMap.emplace_hint(hashMap.end(), "key2", HashTestValue(2, "second"));
    EXPECT_EQ(it->first, "key2");
    EXPECT_EQ(it->second.id, 2);
    EXPECT_EQ(it->second.name, "second");

    EXPECT_EQ(hashMap.size(), 2);
}
