// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashSet.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashSet
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashSet.h"
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <chrono>

/****************************************************************************
 * NFShmHashSet 单元测试
 ****************************************************************************
 * 
 * 测试目标：
 * 1. 验证与std::unordered_set的API兼容性
 * 2. 测试共享内存特有功能（固定容量、CreateInit/ResumeInit等）
 * 3. 验证哈希集合特有的性能特征和行为
 * 4. 测试与STL容器的互操作性
 * 5. 验证元素唯一性保证
 * 6. 测试错误处理和边界条件
 *****************************************************************************/

// 自定义元素类型用于测试
class HashSetTestElement
{
public:
    HashSetTestElement() : value(0), name("default")
    {
        ++constructor_count;
    }

    HashSetTestElement(int v) : value(v), name("element_" + std::to_string(v))
    {
        ++constructor_count;
    }

    HashSetTestElement(int v, const std::string& n) : value(v), name(n)
    {
        ++constructor_count;
    }

    HashSetTestElement(const HashSetTestElement& other) : value(other.value), name(other.name)
    {
        ++constructor_count;
    }

    HashSetTestElement& operator=(const HashSetTestElement& other)
    {
        if (this != &other)
        {
            value = other.value;
            name = other.name;
        }
        return *this;
    }

    ~HashSetTestElement()
    {
        ++destructor_count;
    }

    bool operator==(const HashSetTestElement& other) const
    {
        return value == other.value;
    }

    bool operator<(const HashSetTestElement& other) const
    {
        return value < other.value;
    }

    int value;
    std::string name;

    static int constructor_count;
    static int destructor_count;

    static void reset_counters()
    {
        constructor_count = 0;
        destructor_count = 0;
    }
};

int HashSetTestElement::constructor_count = 0;
int HashSetTestElement::destructor_count = 0;

// 自定义哈希函数
namespace std
{
    template <>
    struct hash<HashSetTestElement>
    {
        std::size_t operator()(const HashSetTestElement& elem) const
        {
            return std::hash<int>()(elem.value);
        }
    };
}

// 自定义字符串哈希函数用于测试
struct CustomStringHasher
{
    std::size_t operator()(const std::string& str) const
    {
        std::size_t hash = 0;
        for (char c : str)
        {
            hash = hash * 31 + c;
        }
        return hash;
    }
};

class NFShmHashSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        HashSetTestElement::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(HashSetTestElement::constructor_count, HashSetTestElement::destructor_count);
    }
};

// ==================== 基本功能测试 ====================

TEST_F(NFShmHashSetTest, BasicOperationsWithInt)
{
    NFShmHashSet<int, 16> hashSet;

    // 测试空容器
    EXPECT_TRUE(hashSet.empty());
    EXPECT_EQ(hashSet.size(), 0);
    EXPECT_EQ(hashSet.max_size(), 16);
    EXPECT_FALSE(hashSet.full());
    EXPECT_EQ(hashSet.left_size(), 16);

    // 测试插入
    auto result = hashSet.insert(10);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 10);
    EXPECT_EQ(hashSet.size(), 1);
    EXPECT_FALSE(hashSet.empty());
    EXPECT_EQ(hashSet.left_size(), 15);

    // 测试重复元素插入（哈希set保证元素唯一性）
    result = hashSet.insert(10);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(*result.first, 10);
    EXPECT_EQ(hashSet.size(), 1); // 大小不变

    // 插入更多元素
    hashSet.insert(20);
    hashSet.insert(30);
    hashSet.insert(40);
    EXPECT_EQ(hashSet.size(), 4);

    // 测试查找
    auto it = hashSet.find(20);
    EXPECT_NE(it, hashSet.end());
    EXPECT_EQ(*it, 20);

    it = hashSet.find(100);
    EXPECT_EQ(it, hashSet.end());

    // 测试count（对于哈希set，count只能返回0或1）
    EXPECT_EQ(hashSet.count(10), 1);
    EXPECT_EQ(hashSet.count(100), 0);

    // 测试equal_range
    auto range = hashSet.equal_range(30);
    EXPECT_NE(range.first, hashSet.end());
    EXPECT_EQ(*range.first, 30);
    EXPECT_EQ(std::distance(range.first, range.second), 1);

    // 测试删除
    size_t erased = hashSet.erase(10);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(hashSet.size(), 3);
    EXPECT_EQ(hashSet.find(10), hashSet.end());

    // 测试迭代器删除
    it = hashSet.find(20);
    EXPECT_NE(it, hashSet.end());
    auto next_it = hashSet.erase(it);
    EXPECT_EQ(hashSet.size(), 2);
    EXPECT_EQ(hashSet.find(20), hashSet.end());
}

// ==================== 字符串元素测试 ====================

TEST_F(NFShmHashSetTest, StringElementOperations)
{
    NFShmHashSet<std::string, 12> hashSet;

    // 测试字符串元素插入
    std::vector<std::string> test_strings = {
        "apple", "banana", "cherry", "date", "elderberry"
    };

    for (const auto& str : test_strings)
    {
        auto result = hashSet.insert(str);
        EXPECT_TRUE(result.second);
        EXPECT_EQ(*result.first, str);
    }
    EXPECT_EQ(hashSet.size(), test_strings.size());

    // 测试重复字符串插入
    auto result = hashSet.insert("apple");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(*result.first, "apple");
    EXPECT_EQ(hashSet.size(), test_strings.size());

    // 测试查找字符串
    for (const auto& str : test_strings)
    {
        auto it = hashSet.find(str);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, str);
    }

    // 测试删除字符串
    hashSet.erase("banana");
    EXPECT_EQ(hashSet.find("banana"), hashSet.end());
    EXPECT_EQ(hashSet.size(), test_strings.size() - 1);
}

// ==================== 自定义类型测试 ====================

TEST_F(NFShmHashSetTest, CustomTypeOperations)
{
    NFShmHashSet<HashSetTestElement, 10> hashSet;

    // 测试自定义类型插入
    HashSetTestElement elem1(1, "first");
    HashSetTestElement elem2(2, "second");
    HashSetTestElement elem3(3, "third");

    auto result = hashSet.insert(elem1);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->value, 1);
    EXPECT_EQ(result.first->name, "first");

    hashSet.insert(elem2);
    hashSet.insert(elem3);
    EXPECT_EQ(hashSet.size(), 3);

    // 测试基于value的唯一性（相同value视为相同元素）
    HashSetTestElement elem1_duplicate(1, "different_name");
    result = hashSet.insert(elem1_duplicate);
    EXPECT_FALSE(result.second); // 应该插入失败
    EXPECT_EQ(hashSet.size(), 3);

    // 测试查找自定义类型
    HashSetTestElement search_elem(2);
    auto it = hashSet.find(search_elem);
    EXPECT_NE(it, hashSet.end());
    EXPECT_EQ(it->value, 2);
    EXPECT_EQ(it->name, "second");

    // 测试删除自定义类型
    hashSet.erase(elem1);
    EXPECT_EQ(hashSet.find(elem1), hashSet.end());
    EXPECT_EQ(hashSet.size(), 2);
}

// ==================== 容量和固定大小测试 ====================

TEST_F(NFShmHashSetTest, CapacityAndFixedSizeOperations)
{
    const size_t MAX_SIZE = 8;
    NFShmHashSet<int, MAX_SIZE> hashSet;

    // 填充到接近容量
    for (int i = 0; i < MAX_SIZE - 1; ++i)
    {
        auto result = hashSet.insert(i);
        EXPECT_TRUE(result.second);
        EXPECT_EQ(hashSet.size(), i + 1);
        EXPECT_EQ(hashSet.left_size(), MAX_SIZE - i - 1);
        EXPECT_FALSE(hashSet.full());
    }

    // 插入最后一个元素
    auto result = hashSet.insert(MAX_SIZE - 1);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(hashSet.size(), MAX_SIZE);
    EXPECT_EQ(hashSet.left_size(), 0);
    EXPECT_TRUE(hashSet.full());

    // 尝试插入超出容量的元素
    result = hashSet.insert(MAX_SIZE);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(hashSet.size(), MAX_SIZE);
    EXPECT_TRUE(hashSet.full());

    // 删除一个元素，验证容量恢复
    hashSet.erase(0);
    EXPECT_EQ(hashSet.size(), MAX_SIZE - 1);
    EXPECT_EQ(hashSet.left_size(), 1);
    EXPECT_FALSE(hashSet.full());

    // 现在可以插入新元素
    result = hashSet.insert(MAX_SIZE);
    EXPECT_TRUE(result.second);
    EXPECT_TRUE(hashSet.full());
}

// ==================== 迭代器测试 ====================

TEST_F(NFShmHashSetTest, IteratorOperations)
{
    NFShmHashSet<int, 15> hashSet;

    // 填充测试数据
    std::vector<int> test_data = {1, 3, 5, 7, 9, 11, 13};

    for (int value : test_data)
    {
        hashSet.insert(value);
    }

    // 测试正向迭代器
    std::vector<int> iterated_data;
    for (auto it = hashSet.begin(); it != hashSet.end(); ++it)
    {
        iterated_data.push_back(*it);
    }
    EXPECT_EQ(iterated_data.size(), test_data.size());

    // 验证所有元素都能通过迭代器访问到
    for (int value : test_data)
    {
        auto found = std::find(iterated_data.begin(), iterated_data.end(), value);
        EXPECT_NE(found, iterated_data.end());
    }

    // 测试const迭代器
    const auto& const_set = hashSet;
    std::vector<int> const_iterated_data;
    for (auto it = const_set.begin(); it != const_set.end(); ++it)
    {
        const_iterated_data.push_back(*it);
    }
    EXPECT_EQ(const_iterated_data.size(), test_data.size());

    // 测试cbegin/cend
    std::vector<int> c_iterated_data;
    for (auto it = hashSet.cbegin(); it != hashSet.cend(); ++it)
    {
        c_iterated_data.push_back(*it);
    }
    EXPECT_EQ(c_iterated_data.size(), test_data.size());

    // 测试范围for循环
    std::vector<int> range_data;
    for (const auto& value : hashSet)
    {
        range_data.push_back(value);
    }
    EXPECT_EQ(range_data.size(), test_data.size());
}

// ==================== STL兼容性测试 ====================

TEST_F(NFShmHashSetTest, STLCompatibility)
{
    // 从std::unordered_set构造
    std::unordered_set<int> stdSet = {1, 2, 3, 4, 5};

    NFShmHashSet<int, 10> hashSet(stdSet);
    EXPECT_EQ(hashSet.size(), 5);

    for (int value : stdSet)
    {
        auto it = hashSet.find(value);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, value);
    }

    // 从std::set构造
    std::set<int> orderedSet = {10, 20, 30, 40};

    NFShmHashSet<int, 8> hashSet2(orderedSet);
    EXPECT_EQ(hashSet2.size(), 4);

    for (int value : orderedSet)
    {
        auto it = hashSet2.find(value);
        EXPECT_NE(it, hashSet2.end());
        EXPECT_EQ(*it, value);
    }

    // 测试赋值操作
    NFShmHashSet<int, 15> hashSet3;
    hashSet3 = stdSet;
    EXPECT_EQ(hashSet3.size(), 5);

    hashSet3 = orderedSet;
    EXPECT_EQ(hashSet3.size(), 4);

    // 测试初始化列表
    NFShmHashSet<int, 12> hashSet4{100, 200, 300};
    EXPECT_EQ(hashSet4.size(), 3);
    EXPECT_EQ(hashSet4.count(100), 1);
    EXPECT_EQ(hashSet4.count(200), 1);
    EXPECT_EQ(hashSet4.count(300), 1);

    // 测试初始化列表赋值
    hashSet4 = {400, 500, 600, 700};
    EXPECT_EQ(hashSet4.size(), 4);
    EXPECT_EQ(hashSet4.count(400), 1);
    EXPECT_EQ(hashSet4.count(500), 1);
}

// ==================== 范围插入测试 ====================

TEST_F(NFShmHashSetTest, RangeInsertOperations)
{
    NFShmHashSet<int, 20> hashSet;

    // 准备测试数据
    std::vector<int> data = {1, 2, 3, 4, 5, 2, 3}; // 包含重复元素

    // 测试迭代器范围插入
    hashSet.insert(data.begin(), data.end());
    EXPECT_EQ(hashSet.size(), 5); // 重复元素被忽略

    for (int i = 1; i <= 5; ++i)
    {
        auto it = hashSet.find(i);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, i);
    }

    // 测试数组范围插入
    int array_data[] = {10, 20, 30, 20, 10};

    hashSet.insert(array_data, array_data + 5);
    EXPECT_EQ(hashSet.size(), 8); // 5个原有 + 3个新增

    for (int value : {10, 20, 30})
    {
        auto it = hashSet.find(value);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, value);
    }
}

// ==================== 删除操作测试 ====================

TEST_F(NFShmHashSetTest, EraseOperations)
{
    NFShmHashSet<int, 15> hashSet;

    // 填充测试数据
    for (int i = 1; i <= 10; ++i)
    {
        hashSet.insert(i);
    }
    EXPECT_EQ(hashSet.size(), 10);

    // 测试按值删除
    size_t erased = hashSet.erase(1);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(hashSet.size(), 9);
    EXPECT_EQ(hashSet.find(1), hashSet.end());

    // 测试删除不存在的元素
    erased = hashSet.erase(100);
    EXPECT_EQ(erased, 0);
    EXPECT_EQ(hashSet.size(), 9);

    // 测试迭代器删除
    auto it = hashSet.find(2);
    EXPECT_NE(it, hashSet.end());
    auto next_it = hashSet.erase(it);
    EXPECT_EQ(hashSet.size(), 8);
    EXPECT_EQ(hashSet.find(2), hashSet.end());

    // 测试const迭代器删除
    auto const_it = hashSet.find(3);
    EXPECT_NE(const_it, hashSet.end());
    hashSet.erase(const_it);
    EXPECT_EQ(hashSet.size(), 7);
    EXPECT_EQ(hashSet.find(3), hashSet.end());

    // 测试范围删除
    auto first = hashSet.find(4);
    auto last = hashSet.find(7);
    if (first != hashSet.end() && last != hashSet.end())
    {
        ++last; // 范围删除是[first, last)
        size_t old_size = hashSet.size();
        auto result_it = hashSet.erase(first, last);
        EXPECT_LT(hashSet.size(), old_size);
    }

    // 测试清空
    hashSet.clear();
    EXPECT_TRUE(hashSet.empty());
    EXPECT_EQ(hashSet.size(), 0);
}

// ==================== 自定义哈希函数测试 ====================

TEST_F(NFShmHashSetTest, CustomHashFunction)
{
    NFShmHashSet<std::string, 12, CustomStringHasher> hashSet;

    // 测试自定义哈希函数
    std::vector<std::string> test_strings = {"hello", "world", "test", "custom"};

    for (const auto& str : test_strings)
    {
        hashSet.insert(str);
    }

    EXPECT_EQ(hashSet.size(), test_strings.size());

    // 验证哈希函数正常工作
    for (const auto& str : test_strings)
    {
        auto it = hashSet.find(str);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, str);
    }
}

// ==================== 性能和压力测试 ====================

TEST_F(NFShmHashSetTest, PerformanceAndStressTest)
{
    const size_t LARGE_SIZE = 1000;
    NFShmHashSet<int, LARGE_SIZE> hashSet;

    // 大量插入测试
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        hashSet.insert(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(hashSet.size(), LARGE_SIZE);
    EXPECT_TRUE(hashSet.full());

    // 验证所有元素都存在
    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        auto it = hashSet.find(i);
        EXPECT_NE(it, hashSet.end());
        EXPECT_EQ(*it, i);
    }

    // 大量查找测试
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        EXPECT_EQ(hashSet.count(i), 1);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 大量删除测试
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE / 2; ++i)
    {
        hashSet.erase(i);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(hashSet.size(), LARGE_SIZE / 2);
}

// ==================== 共享内存特有功能测试 ====================

TEST_F(NFShmHashSetTest, SharedMemorySpecificFeatures)
{
    NFShmHashSet<int, 10> hashSet;

    // 测试CreateInit和ResumeInit
    EXPECT_EQ(hashSet.CreateInit(), 0);
    EXPECT_EQ(hashSet.ResumeInit(), 0);

    // 测试Init方法
    hashSet.insert(1);
    hashSet.insert(2);
    EXPECT_EQ(hashSet.size(), 2);

    hashSet.Init();
    EXPECT_EQ(hashSet.size(), 0);
    EXPECT_TRUE(hashSet.empty());

    // 测试桶接口
    EXPECT_EQ(hashSet.bucket_count(), 10);
    EXPECT_EQ(hashSet.max_bucket_count(), 10);

    // 填充一些数据测试桶
    for (int i = 0; i < 5; ++i)
    {
        hashSet.insert(i);
    }

    // 测试桶中元素数量
    size_t total_elements = 0;
    for (size_t i = 0; i < hashSet.bucket_count(); ++i)
    {
        total_elements += hashSet.elems_in_bucket(i);
    }
    EXPECT_EQ(total_elements, hashSet.size());

    // 测试resize提示（实际不改变大小）
    size_t old_bucket_count = hashSet.bucket_count();
    hashSet.resize(20);
    EXPECT_EQ(hashSet.bucket_count(), old_bucket_count);
}

// ==================== 比较操作符测试 ====================

TEST_F(NFShmHashSetTest, ComparisonOperators)
{
    NFShmHashSet<int, 10> hashSet1;
    NFShmHashSet<int, 10> hashSet2;

    // 空容器比较
    EXPECT_TRUE(hashSet1 == hashSet2);

    // 添加相同元素
    hashSet1.insert(1);
    hashSet1.insert(2);
    hashSet1.insert(3);

    hashSet2.insert(1);
    hashSet2.insert(2);
    hashSet2.insert(3);

    EXPECT_TRUE(hashSet1 == hashSet2);

    // 添加不同元素
    hashSet2.insert(4);
    EXPECT_FALSE(hashSet1 == hashSet2);

    // 不同的元素集合
    hashSet1.clear();
    hashSet2.clear();

    hashSet1.insert(1);
    hashSet1.insert(2);

    hashSet2.insert(3);
    hashSet2.insert(4);

    EXPECT_FALSE(hashSet1 == hashSet2);
}

// ==================== 交换操作测试 ====================

TEST_F(NFShmHashSetTest, SwapOperations)
{
    NFShmHashSet<int, 10> hashSet1;
    NFShmHashSet<int, 10> hashSet2;

    // 准备测试数据
    hashSet1.insert(1);
    hashSet1.insert(2);

    hashSet2.insert(10);
    hashSet2.insert(20);
    hashSet2.insert(30);

    size_t size1 = hashSet1.size();
    size_t size2 = hashSet2.size();

    // 测试成员函数swap
    hashSet1.swap(hashSet2);

    EXPECT_EQ(hashSet1.size(), size2);
    EXPECT_EQ(hashSet2.size(), size1);

    EXPECT_EQ(hashSet1.count(10), 1);
    EXPECT_EQ(hashSet1.count(20), 1);
    EXPECT_EQ(hashSet1.count(30), 1);

    EXPECT_EQ(hashSet2.count(1), 1);
    EXPECT_EQ(hashSet2.count(2), 1);

    // 测试全局swap函数（假设存在）
    std::swap(hashSet1, hashSet2);

    EXPECT_EQ(hashSet1.size(), size1);
    EXPECT_EQ(hashSet2.size(), size2);

    EXPECT_EQ(hashSet1.count(1), 1);
    EXPECT_EQ(hashSet1.count(2), 1);

    EXPECT_EQ(hashSet2.count(10), 1);
    EXPECT_EQ(hashSet2.count(20), 1);
    EXPECT_EQ(hashSet2.count(30), 1);
}

// ==================== emplace功能测试 ====================

TEST_F(NFShmHashSetTest, EmplaceOperations)
{
    NFShmHashSet<HashSetTestElement, 10> hashSet;

    // 测试emplace
    auto result = hashSet.emplace(1, "first");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->value, 1);
    EXPECT_EQ(result.first->name, "first");

    // 测试重复元素的emplace
    result = hashSet.emplace(1, "duplicate");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->value, 1);
    EXPECT_EQ(result.first->name, "first"); // 保持原值

    // 测试emplace_hint
    auto it = hashSet.emplace_hint(hashSet.end(), 2, "second");
    EXPECT_EQ(it->value, 2);
    EXPECT_EQ(it->name, "second");

    EXPECT_EQ(hashSet.size(), 2);
}
