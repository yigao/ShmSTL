// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashMultiSet.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashMultiSet
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashMultiSet.h"
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <chrono>

/****************************************************************************
 * NFShmHashMultiSet 单元测试
 ****************************************************************************
 * 
 * 测试目标：
 * 1. 验证与std::unordered_multiset的API兼容性
 * 2. 测试允许重复元素的多重集合特性
 * 3. 测试共享内存特有功能（固定容量、CreateInit/ResumeInit等）
 * 4. 验证哈希集合特有的性能特征和行为
 * 5. 测试与STL容器的互操作性
 * 6. 验证与NFShmHashSet的差异（重复 vs 唯一）
 * 7. 测试错误处理和边界条件
 *****************************************************************************/

// 自定义元素类型用于测试
class HashMultiSetTestElement
{
public:
    HashMultiSetTestElement() : value(0), name("default")
    {
        ++constructor_count;
    }

    HashMultiSetTestElement(int v) : value(v), name("element_" + std::to_string(v))
    {
        ++constructor_count;
    }

    HashMultiSetTestElement(int v, const std::string& n) : value(v), name(n)
    {
        ++constructor_count;
    }

    HashMultiSetTestElement(const HashMultiSetTestElement& other) : value(other.value), name(other.name)
    {
        ++constructor_count;
    }

    HashMultiSetTestElement& operator=(const HashMultiSetTestElement& other)
    {
        if (this != &other)
        {
            value = other.value;
            name = other.name;
        }
        return *this;
    }

    ~HashMultiSetTestElement()
    {
        ++destructor_count;
    }

    bool operator==(const HashMultiSetTestElement& other) const
    {
        return value == other.value; // 基于value进行比较
    }

    bool operator<(const HashMultiSetTestElement& other) const
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

int HashMultiSetTestElement::constructor_count = 0;
int HashMultiSetTestElement::destructor_count = 0;

// 自定义哈希函数
namespace std
{
    template <>
    struct hash<HashMultiSetTestElement>
    {
        std::size_t operator()(const HashMultiSetTestElement& elem) const
        {
            return std::hash<int>()(elem.value);
        }
    };
}

// 自定义字符串哈希函数用于测试
struct CustomMultiSetStringHasher
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

class NFShmHashMultiSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        HashMultiSetTestElement::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(HashMultiSetTestElement::constructor_count, HashMultiSetTestElement::destructor_count);
    }
};

// ==================== 基本功能测试 ====================

TEST_F(NFShmHashMultiSetTest, BasicOperationsWithInt)
{
    NFShmHashMultiSet<int, 16> multiSet;

    // 测试空容器
    EXPECT_TRUE(multiSet.empty());
    EXPECT_EQ(multiSet.size(), 0);
    EXPECT_EQ(multiSet.max_size(), 16);
    EXPECT_FALSE(multiSet.full());
    EXPECT_EQ(multiSet.left_size(), 16);

    // 测试插入（多重集合总是成功插入）
    auto it = multiSet.insert(10);
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(multiSet.size(), 1);
    EXPECT_FALSE(multiSet.empty());
    EXPECT_EQ(multiSet.left_size(), 15);

    // 测试重复元素插入（与unordered_set不同，多重集合允许重复元素）
    it = multiSet.insert(10);
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(multiSet.size(), 2); // 大小增加

    // 再次插入相同元素
    multiSet.insert(10);
    EXPECT_EQ(multiSet.size(), 3);

    // 插入不同元素
    multiSet.insert(20);
    multiSet.insert(30);
    multiSet.insert(20); // 再次插入20
    EXPECT_EQ(multiSet.size(), 6);

    // 测试查找（返回第一个匹配的元素）
    auto find_it = multiSet.find(10);
    EXPECT_NE(find_it, multiSet.end());
    EXPECT_EQ(*find_it, 10);

    find_it = multiSet.find(100);
    EXPECT_EQ(find_it, multiSet.end());

    // 测试count（对于多重集合，count可以返回>1）
    EXPECT_EQ(multiSet.count(10), 3); // 元素10有3个副本
    EXPECT_EQ(multiSet.count(20), 2); // 元素20有2个副本
    EXPECT_EQ(multiSet.count(30), 1); // 元素30有1个副本
    EXPECT_EQ(multiSet.count(100), 0); // 元素100不存在

    // 测试equal_range
    auto range = multiSet.equal_range(10);
    size_t count = 0;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        EXPECT_EQ(*iter, 10);
        ++count;
    }
    EXPECT_EQ(count, 3); // 应该有3个元素

    // 测试删除（删除所有匹配的元素）
    size_t erased = multiSet.erase(10);
    EXPECT_EQ(erased, 3); // 删除了3个元素
    EXPECT_EQ(multiSet.size(), 3);
    EXPECT_EQ(multiSet.find(10), multiSet.end());

    // 测试迭代器删除（只删除单个元素）
    find_it = multiSet.find(20);
    EXPECT_NE(find_it, multiSet.end());
    auto next_it = multiSet.erase(find_it);
    EXPECT_EQ(multiSet.size(), 2);
    EXPECT_EQ(multiSet.count(20), 1); // 还剩1个元素20
}

// ==================== 多重元素测试 ====================

TEST_F(NFShmHashMultiSetTest, MultipleElementsOperations)
{
    NFShmHashMultiSet<std::string, 20> multiSet;

    // 插入多个相同的元素
    std::string element = "repeated";
    multiSet.insert(element);
    multiSet.insert(element);
    multiSet.insert(element);
    multiSet.insert(element);
    multiSet.insert(element);

    EXPECT_EQ(multiSet.size(), 5);
    EXPECT_EQ(multiSet.count(element), 5);

    // 测试equal_range遍历所有相同的元素
    auto range = multiSet.equal_range(element);
    size_t count = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(*it, element);
        ++count;
    }
    EXPECT_EQ(count, 5);

    // 测试插入其他元素
    multiSet.insert("other");
    multiSet.insert("other");
    multiSet.insert("different");
    EXPECT_EQ(multiSet.size(), 8);
    EXPECT_EQ(multiSet.count("other"), 2);
    EXPECT_EQ(multiSet.count("different"), 1);
    EXPECT_EQ(multiSet.count(element), 5);

    // 测试删除一个元素的所有副本
    size_t erased = multiSet.erase(element);
    EXPECT_EQ(erased, 5);
    EXPECT_EQ(multiSet.size(), 3);
    EXPECT_EQ(multiSet.count(element), 0);
    EXPECT_EQ(multiSet.count("other"), 2);
    EXPECT_EQ(multiSet.count("different"), 1);
}

// ==================== 自定义类型测试 ====================

TEST_F(NFShmHashMultiSetTest, CustomTypeOperations)
{
    NFShmHashMultiSet<HashMultiSetTestElement, 15> multiSet;

    // 测试自定义类型插入
    HashMultiSetTestElement elem1(1, "first");
    HashMultiSetTestElement elem2(2, "second");
    HashMultiSetTestElement elem3(1, "first_duplicate"); // 相同value，不同name

    auto it = multiSet.insert(elem1);
    EXPECT_EQ(it->value, 1);
    EXPECT_EQ(it->name, "first");

    // 插入多个相同value的元素
    multiSet.insert(elem2);
    multiSet.insert(elem3); // 基于value的比较，这应该被视为重复元素
    multiSet.insert(elem1); // 再次插入elem1

    EXPECT_EQ(multiSet.size(), 4);
    EXPECT_EQ(multiSet.count(elem1), 3); // 应该有3个value为1的元素
    EXPECT_EQ(multiSet.count(elem2), 1); // 应该有1个value为2的元素

    // 测试查找自定义类型
    HashMultiSetTestElement search_elem(1);
    auto find_it = multiSet.find(search_elem);
    EXPECT_NE(find_it, multiSet.end());
    EXPECT_EQ(find_it->value, 1);

    // 遍历所有相同value的元素
    auto range = multiSet.equal_range(search_elem);
    std::vector<std::string> names;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        names.push_back(iter->name);
    }
    EXPECT_EQ(names.size(), 3);
}

// ==================== 容量和固定大小测试 ====================

TEST_F(NFShmHashMultiSetTest, CapacityAndFixedSizeOperations)
{
    const size_t MAX_SIZE = 8;
    NFShmHashMultiSet<int, MAX_SIZE> multiSet;

    // 填充到接近容量（使用重复元素）
    for (int i = 0; i < MAX_SIZE - 1; ++i)
    {
        auto it = multiSet.insert(i % 3); // 使用模运算创建重复元素
        EXPECT_EQ(multiSet.size(), i + 1);
        EXPECT_EQ(multiSet.left_size(), MAX_SIZE - i - 1);
        EXPECT_FALSE(multiSet.full());
    }

    // 插入最后一个元素
    auto it = multiSet.insert(100);
    EXPECT_EQ(multiSet.size(), MAX_SIZE);
    EXPECT_EQ(multiSet.left_size(), 0);
    EXPECT_TRUE(multiSet.full());

    // 尝试插入超出容量的元素（应该失败或者不增加大小）
    auto failed_it = multiSet.insert(200);
    EXPECT_EQ(multiSet.size(), MAX_SIZE);
    EXPECT_TRUE(multiSet.full());

    // 删除一个元素，验证容量恢复
    multiSet.erase(100);
    EXPECT_LT(multiSet.size(), MAX_SIZE);
    EXPECT_GT(multiSet.left_size(), 0);
    EXPECT_FALSE(multiSet.full());

    // 现在可以插入新元素
    it = multiSet.insert(300);
    EXPECT_EQ(*it, 300);
}

// ==================== 迭代器测试 ====================

TEST_F(NFShmHashMultiSetTest, IteratorOperations)
{
    NFShmHashMultiSet<int, 20> multiSet;

    // 填充测试数据（包含重复元素）
    std::vector<int> test_data = {1, 1, 2, 2, 2, 3, 4, 4, 5};

    for (int value : test_data)
    {
        multiSet.insert(value);
    }
    EXPECT_EQ(multiSet.size(), test_data.size());

    // 测试正向迭代器
    std::vector<int> iterated_data;
    for (auto it = multiSet.begin(); it != multiSet.end(); ++it)
    {
        iterated_data.push_back(*it);
    }
    EXPECT_EQ(iterated_data.size(), test_data.size());

    // 验证所有元素都能通过迭代器访问到（包括重复元素）
    std::sort(iterated_data.begin(), iterated_data.end());
    std::sort(test_data.begin(), test_data.end());
    EXPECT_EQ(iterated_data, test_data);

    // 测试const迭代器
    const auto& const_set = multiSet;
    std::vector<int> const_iterated_data;
    for (auto it = const_set.begin(); it != const_set.end(); ++it)
    {
        const_iterated_data.push_back(*it);
    }
    EXPECT_EQ(const_iterated_data.size(), test_data.size());

    // 测试cbegin/cend
    std::vector<int> c_iterated_data;
    for (auto it = multiSet.cbegin(); it != multiSet.cend(); ++it)
    {
        c_iterated_data.push_back(*it);
    }
    EXPECT_EQ(c_iterated_data.size(), test_data.size());

    // 测试范围for循环
    std::vector<int> range_data;
    for (const auto& value : multiSet)
    {
        range_data.push_back(value);
    }
    EXPECT_EQ(range_data.size(), test_data.size());
}

// ==================== STL兼容性测试 ====================

TEST_F(NFShmHashMultiSetTest, STLCompatibility)
{
    // 从std::unordered_multiset构造
    std::unordered_multiset<int> stdMultiSet = {1, 1, 2, 2, 2, 3, 4, 4, 5};

    NFShmHashMultiSet<int, 15> multiSet(stdMultiSet);
    EXPECT_EQ(multiSet.size(), 9);

    // 验证count
    EXPECT_EQ(multiSet.count(1), 2);
    EXPECT_EQ(multiSet.count(2), 3);
    EXPECT_EQ(multiSet.count(3), 1);
    EXPECT_EQ(multiSet.count(4), 2);
    EXPECT_EQ(multiSet.count(5), 1);

    // 从std::multiset构造
    std::multiset<int> orderedMultiSet = {10, 10, 20, 20, 30};

    NFShmHashMultiSet<int, 12> multiSet2(orderedMultiSet);
    EXPECT_EQ(multiSet2.size(), 5);
    EXPECT_EQ(multiSet2.count(10), 2);
    EXPECT_EQ(multiSet2.count(20), 2);
    EXPECT_EQ(multiSet2.count(30), 1);

    // 测试赋值操作
    NFShmHashMultiSet<int, 20> multiSet3;
    multiSet3 = stdMultiSet;
    EXPECT_EQ(multiSet3.size(), 9);

    multiSet3 = orderedMultiSet;
    EXPECT_EQ(multiSet3.size(), 5);

    // 测试初始化列表
    NFShmHashMultiSet<int, 15> multiSet4{100, 100, 100, 200, 200};
    EXPECT_EQ(multiSet4.size(), 5);
    EXPECT_EQ(multiSet4.count(100), 3);
    EXPECT_EQ(multiSet4.count(200), 2);

    // 测试初始化列表赋值
    multiSet4 = {300, 300, 400, 400, 400, 500};
    EXPECT_EQ(multiSet4.size(), 6);
    EXPECT_EQ(multiSet4.count(300), 2);
    EXPECT_EQ(multiSet4.count(400), 3);
    EXPECT_EQ(multiSet4.count(500), 1);
}

// ==================== 范围插入测试 ====================

TEST_F(NFShmHashMultiSetTest, RangeInsertOperations)
{
    NFShmHashMultiSet<int, 25> multiSet;

    // 准备测试数据（包含重复元素）
    std::vector<int> data = {1, 1, 2, 2, 3, 3, 3, 4, 5, 5};

    // 测试迭代器范围插入
    multiSet.insert(data.begin(), data.end());
    EXPECT_EQ(multiSet.size(), 10); // 所有元素都应该被插入

    // 验证count
    EXPECT_EQ(multiSet.count(1), 2);
    EXPECT_EQ(multiSet.count(2), 2);
    EXPECT_EQ(multiSet.count(3), 3);
    EXPECT_EQ(multiSet.count(4), 1);
    EXPECT_EQ(multiSet.count(5), 2);

    // 测试数组范围插入
    int array_data[] = {10, 10, 20, 20, 20};

    multiSet.insert(array_data, array_data + 5);
    EXPECT_EQ(multiSet.size(), 15);
    EXPECT_EQ(multiSet.count(10), 2);
    EXPECT_EQ(multiSet.count(20), 3);
}

// ==================== 删除操作测试 ====================

TEST_F(NFShmHashMultiSetTest, EraseOperations)
{
    NFShmHashMultiSet<int, 20> multiSet;

    // 填充测试数据（包含重复元素）
    for (int i = 1; i <= 5; ++i)
    {
        multiSet.insert(i);
        multiSet.insert(i); // 每个元素插入两次
    }
    EXPECT_EQ(multiSet.size(), 10);

    // 测试按值删除（删除所有匹配的元素）
    size_t erased = multiSet.erase(1);
    EXPECT_EQ(erased, 2); // 删除了2个元素
    EXPECT_EQ(multiSet.size(), 8);
    EXPECT_EQ(multiSet.find(1), multiSet.end());

    // 测试删除不存在的元素
    erased = multiSet.erase(100);
    EXPECT_EQ(erased, 0);
    EXPECT_EQ(multiSet.size(), 8);

    // 测试迭代器删除（只删除单个元素）
    auto it = multiSet.find(2);
    EXPECT_NE(it, multiSet.end());
    auto next_it = multiSet.erase(it);
    EXPECT_EQ(multiSet.size(), 7);
    EXPECT_EQ(multiSet.count(2), 1); // 还剩1个元素2

    // 测试const迭代器删除
    auto const_it = multiSet.find(3);
    EXPECT_NE(const_it, multiSet.end());
    multiSet.erase(const_it);
    EXPECT_EQ(multiSet.size(), 6);
    EXPECT_EQ(multiSet.count(3), 1);

    // 测试范围删除
    auto range = multiSet.equal_range(4);
    if (range.first != range.second)
    {
        size_t old_size = multiSet.size();
        auto result_it = multiSet.erase(range.first, range.second);
        EXPECT_LT(multiSet.size(), old_size);
        EXPECT_EQ(multiSet.count(4), 0);
    }

    // 测试清空
    multiSet.clear();
    EXPECT_TRUE(multiSet.empty());
    EXPECT_EQ(multiSet.size(), 0);
}

// ==================== 自定义哈希函数测试 ====================

TEST_F(NFShmHashMultiSetTest, CustomHashFunction)
{
    NFShmHashMultiSet<std::string, 15, CustomMultiSetStringHasher> multiSet;

    // 测试自定义哈希函数
    std::vector<std::string> test_strings = {"hello", "hello", "world", "world", "test"};

    for (const auto& str : test_strings)
    {
        multiSet.insert(str);
    }

    EXPECT_EQ(multiSet.size(), test_strings.size());
    EXPECT_EQ(multiSet.count("hello"), 2);
    EXPECT_EQ(multiSet.count("world"), 2);
    EXPECT_EQ(multiSet.count("test"), 1);

    // 验证哈希函数正常工作
    auto range = multiSet.equal_range("hello");
    size_t count = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(*it, "hello");
        ++count;
    }
    EXPECT_EQ(count, 2);
}

// ==================== 性能和压力测试 ====================

TEST_F(NFShmHashMultiSetTest, PerformanceAndStressTest)
{
    const size_t LARGE_SIZE = 1000;
    NFShmHashMultiSet<int, LARGE_SIZE> multiSet;

    // 大量插入测试（创建重复元素）
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < LARGE_SIZE; ++i)
    {
        multiSet.insert(i % 100); // 创建重复元素
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(multiSet.size(), LARGE_SIZE);
    EXPECT_TRUE(multiSet.full());

    // 验证count功能
    for (int elem = 0; elem < 100; ++elem)
    {
        EXPECT_EQ(multiSet.count(elem), 10); // 每个元素应该有10个副本
    }

    // 大量查找测试
    start = std::chrono::high_resolution_clock::now();

    for (int elem = 0; elem < 100; ++elem)
    {
        auto range = multiSet.equal_range(elem);
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

    for (int elem = 0; elem < 50; ++elem)
    {
        multiSet.erase(elem);
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_EQ(multiSet.size(), LARGE_SIZE / 2);
}

// ==================== 共享内存特有功能测试 ====================

TEST_F(NFShmHashMultiSetTest, SharedMemorySpecificFeatures)
{
    NFShmHashMultiSet<int, 12> multiSet;

    // 测试CreateInit和ResumeInit
    EXPECT_EQ(multiSet.CreateInit(), 0);
    EXPECT_EQ(multiSet.ResumeInit(), 0);

    // 测试Init方法
    multiSet.insert(1);
    multiSet.insert(1);
    multiSet.insert(2);
    EXPECT_EQ(multiSet.size(), 3);

    multiSet.Init();
    EXPECT_EQ(multiSet.size(), 0);
    EXPECT_TRUE(multiSet.empty());

    // 测试桶接口
    EXPECT_EQ(multiSet.bucket_count(), 12);
    EXPECT_EQ(multiSet.max_bucket_count(), 12);

    // 填充一些数据测试桶
    for (int i = 0; i < 6; ++i)
    {
        multiSet.insert(i % 3);
    }

    // 测试桶中元素数量
    size_t total_elements = 0;
    for (size_t i = 0; i < multiSet.bucket_count(); ++i)
    {
        total_elements += multiSet.elems_in_bucket(i);
    }
    EXPECT_EQ(total_elements, multiSet.size());

    // 测试resize提示（实际不改变大小）
    size_t old_bucket_count = multiSet.bucket_count();
    multiSet.resize(20);
    EXPECT_EQ(multiSet.bucket_count(), old_bucket_count);
}

// ==================== 比较操作符测试 ====================

TEST_F(NFShmHashMultiSetTest, ComparisonOperators)
{
    NFShmHashMultiSet<int, 15> multiSet1;
    NFShmHashMultiSet<int, 15> multiSet2;

    // 空容器比较
    EXPECT_TRUE(multiSet1 == multiSet2);

    // 添加相同元素（包括重复元素）
    multiSet1.insert(1);
    multiSet1.insert(1);
    multiSet1.insert(2);
    multiSet1.insert(3);

    multiSet2.insert(1);
    multiSet2.insert(1);
    multiSet2.insert(2);
    multiSet2.insert(3);

    EXPECT_TRUE(multiSet1 == multiSet2);

    // 添加不同元素
    multiSet2.insert(4);
    EXPECT_FALSE(multiSet1 == multiSet2);

    // 不同的重复次数
    multiSet1.insert(2); // 现在multiSet1有2个元素2
    EXPECT_FALSE(multiSet1 == multiSet2);
}

// ==================== 交换操作测试 ====================

TEST_F(NFShmHashMultiSetTest, SwapOperations)
{
    NFShmHashMultiSet<int, 15> multiSet1;
    NFShmHashMultiSet<int, 15> multiSet2;

    // 准备测试数据
    multiSet1.insert(1);
    multiSet1.insert(1);
    multiSet1.insert(2);

    multiSet2.insert(10);
    multiSet2.insert(10);
    multiSet2.insert(10);
    multiSet2.insert(20);

    size_t size1 = multiSet1.size();
    size_t size2 = multiSet2.size();

    // 测试成员函数swap
    multiSet1.swap(multiSet2);

    EXPECT_EQ(multiSet1.size(), size2);
    EXPECT_EQ(multiSet2.size(), size1);

    EXPECT_EQ(multiSet1.count(10), 3);
    EXPECT_EQ(multiSet1.count(20), 1);
    EXPECT_EQ(multiSet1.count(1), 0);

    EXPECT_EQ(multiSet2.count(1), 2);
    EXPECT_EQ(multiSet2.count(2), 1);
    EXPECT_EQ(multiSet2.count(10), 0);
}

// ==================== emplace功能测试 ====================

TEST_F(NFShmHashMultiSetTest, EmplaceOperations)
{
    NFShmHashMultiSet<HashMultiSetTestElement, 12> multiSet;

    // 测试emplace（多重集合总是成功）
    auto it = multiSet.emplace(1, "first");
    EXPECT_EQ(it->value, 1);
    EXPECT_EQ(it->name, "first");

    // 测试重复元素的emplace（应该成功，因为是multiset）
    it = multiSet.emplace(1, "duplicate");
    EXPECT_EQ(it->value, 1);
    EXPECT_EQ(it->name, "duplicate");

    // 测试emplace_hint
    auto hint_it = multiSet.emplace_hint(multiSet.end(), 2, "second");
    EXPECT_EQ(hint_it->value, 2);
    EXPECT_EQ(hint_it->name, "second");

    EXPECT_EQ(multiSet.size(), 3);
    EXPECT_EQ(multiSet.count(HashMultiSetTestElement(1)), 2);
    EXPECT_EQ(multiSet.count(HashMultiSetTestElement(2)), 1);
}

// ==================== 与NFShmHashSet差异测试 ====================

TEST_F(NFShmHashMultiSetTest, DifferenceFromNFShmHashSet)
{
    NFShmHashMultiSet<int, 10> multiSet;

    // NFShmHashMultiSet允许重复元素
    multiSet.insert(1);
    multiSet.insert(1);
    multiSet.insert(1);
    EXPECT_EQ(multiSet.size(), 3);
    EXPECT_EQ(multiSet.count(1), 3);

    // 相比之下，NFShmHashSet不允许重复元素
    // NFShmHashSet<int, 10> hashSet;
    // hashSet.insert(1);
    // hashSet.insert(1); // 这不会增加size
    // EXPECT_EQ(hashSet.size(), 1);
    // EXPECT_EQ(hashSet.count(1), 1);

    // 测试equal_range的行为差异
    auto range = multiSet.equal_range(1);
    size_t count = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        ++count;
    }
    EXPECT_EQ(count, 3); // multiset返回所有副本

    // 对于hashset，equal_range最多返回1个元素
}

// ==================== 边界条件测试 ====================

TEST_F(NFShmHashMultiSetTest, EdgeCaseOperations)
{
    NFShmHashMultiSet<int, 5> smallMultiSet;

    // 测试在很小的容器中插入大量重复元素
    for (int i = 0; i < 5; ++i)
    {
        smallMultiSet.insert(1); // 全部插入同一个元素
    }
    EXPECT_EQ(smallMultiSet.size(), 5);
    EXPECT_EQ(smallMultiSet.count(1), 5);
    EXPECT_TRUE(smallMultiSet.full());

    // 测试删除所有重复元素
    size_t erased = smallMultiSet.erase(1);
    EXPECT_EQ(erased, 5);
    EXPECT_TRUE(smallMultiSet.empty());

    // 测试插入不同元素到满容量
    smallMultiSet.insert(1);
    smallMultiSet.insert(2);
    smallMultiSet.insert(3);
    smallMultiSet.insert(4);
    smallMultiSet.insert(5);
    EXPECT_TRUE(smallMultiSet.full());

    // 测试在满容量时再次插入
    auto it = smallMultiSet.insert(6);
    EXPECT_EQ(smallMultiSet.size(), 5); // 大小不应该增加
}
