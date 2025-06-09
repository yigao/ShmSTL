// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmSet.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmSet
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmSet.h"
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>

// 自定义测试类
class TestElement
{
public:
    TestElement() : value(0), name("default")
    {
        ++constructor_count;
    }

    TestElement(int v) : value(v), name("value_" + std::to_string(v))
    {
        ++constructor_count;
    }

    TestElement(const int v, const std::string& n) : value(v), name(n)
    {
        ++constructor_count;
    }

    TestElement(const TestElement& other) : value(other.value), name(other.name)
    {
        ++constructor_count;
    }

    TestElement& operator=(const TestElement& other)
    {
        if (this != &other)
        {
            value = other.value;
            name = other.name;
        }
        return *this;
    }

    ~TestElement()
    {
        ++destructor_count;
    }

    bool operator==(const TestElement& other) const
    {
        return value == other.value;
    }

    bool operator<(const TestElement& other) const
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

int TestElement::constructor_count = 0;
int TestElement::destructor_count = 0;

class NFShmSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestElement::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(TestElement::constructor_count, TestElement::destructor_count);
    }
};

// 基本功能测试
TEST_F(NFShmSetTest, BasicOperationsWithInt)
{
    NFShmSet<int, 10> set;

    // 测试空容器
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);
    EXPECT_EQ(set.max_size(), 10);
    EXPECT_FALSE(set.full());

    // 测试插入
    auto result = set.insert(5);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, 5);
    EXPECT_EQ(set.size(), 1);
    EXPECT_FALSE(set.empty());

    // 测试重复插入
    result = set.insert(5);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(*result.first, 5);
    EXPECT_EQ(set.size(), 1);

    // 测试多个元素插入
    set.insert(1);
    set.insert(3);
    set.insert(7);
    set.insert(9);
    EXPECT_EQ(set.size(), 5);

    // 测试查找
    auto it = set.find(5);
    EXPECT_NE(it, set.end());
    EXPECT_EQ(*it, 5);

    it = set.find(10);
    EXPECT_EQ(it, set.end());

    // 测试count
    EXPECT_EQ(set.count(5), 1);
    EXPECT_EQ(set.count(10), 0);

    // 测试lower_bound和upper_bound
    auto lower = set.lower_bound(5);
    auto upper = set.upper_bound(5);
    EXPECT_NE(lower, set.end());
    EXPECT_EQ(*lower, 5);
    EXPECT_NE(upper, lower);

    // 测试equal_range
    auto range = set.equal_range(5);
    EXPECT_EQ(range.first, lower);
    EXPECT_EQ(range.second, upper);
    EXPECT_EQ(std::distance(range.first, range.second), 1);

    // 测试删除
    size_t erased = set.erase(5);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(set.size(), 4);
    EXPECT_EQ(set.find(5), set.end());

    // 测试迭代器删除
    it = set.find(3);
    EXPECT_NE(it, set.end());
    set.erase(it);
    EXPECT_EQ(set.size(), 3);
    EXPECT_EQ(set.find(3), set.end());
}

// 字符串类型测试
TEST_F(NFShmSetTest, StringOperations)
{
    NFShmSet<std::string, 5> set;

    // 测试字符串插入
    set.insert("hello");
    set.insert("world");
    set.insert("test");
    EXPECT_EQ(set.size(), 3);

    // 测试字符串排序（应该按字典序排列）
    std::vector<std::string> expected = {"hello", "test", "world"};
    std::vector<std::string> actual;
    for (const auto& str : set)
    {
        actual.push_back(str);
    }
    EXPECT_EQ(actual, expected);

    // 测试字符串查找
    EXPECT_NE(set.find("hello"), set.end());
    EXPECT_EQ(set.find("notfound"), set.end());

    // 测试字符串删除
    set.erase("hello");
    EXPECT_EQ(set.find("hello"), set.end());
    EXPECT_EQ(set.size(), 2);
}

// 自定义类型测试
TEST_F(NFShmSetTest, CustomTypeOperations)
{
    NFShmSet<TestElement, 8> set;

    // 测试自定义类型插入
    TestElement elem1(1, "first");
    TestElement elem2(2, "second");
    TestElement elem3(3, "third");

    auto result = set.insert(elem1);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->value, 1);

    set.insert(elem2);
    set.insert(elem3);
    EXPECT_EQ(set.size(), 3);

    // 测试自定义类型排序
    std::vector<int> expected_values = {1, 2, 3};
    std::vector<int> actual_values;
    for (const auto& elem : set)
    {
        actual_values.push_back(elem.value);
    }
    EXPECT_EQ(actual_values, expected_values);

    // 测试自定义类型查找
    TestElement search_elem(2);
    auto it = set.find(search_elem);
    EXPECT_NE(it, set.end());
    EXPECT_EQ(it->value, 2);

    // 测试自定义类型删除
    set.erase(search_elem);
    EXPECT_EQ(set.find(search_elem), set.end());
    EXPECT_EQ(set.size(), 2);
}

// 构造函数测试
TEST_F(NFShmSetTest, ConstructorTests)
{
    // 默认构造函数
    NFShmSet<int, 5> set1;
    EXPECT_TRUE(set1.empty());

    // 范围构造函数（迭代器）
    std::vector<int> vec = {1, 3, 5, 7, 9};
    NFShmSet<int, 10> set2(vec.begin(), vec.end());
    EXPECT_EQ(set2.size(), 5);
    for (int val : vec)
    {
        EXPECT_NE(set2.find(val), set2.end());
    }

    // 拷贝构造函数
    NFShmSet<int, 10> set3(set2);
    EXPECT_EQ(set3.size(), set2.size());
    EXPECT_TRUE(std::equal(set2.begin(), set2.end(), set3.begin()));

    // 指针范围构造函数
    int arr[] = {2, 4, 6, 8};
    NFShmSet<int, 10> set4(arr, arr + 4);
    EXPECT_EQ(set4.size(), 4);
    for (int val : arr)
    {
        EXPECT_NE(set4.find(val), set4.end());
    }

    // const_iterator范围构造函数
    NFShmSet<int, 10> set5(set2.begin(), set2.end());
    EXPECT_EQ(set5.size(), set2.size());
    EXPECT_TRUE(std::equal(set2.begin(), set2.end(), set5.begin()));
}

// STL兼容性测试
TEST_F(NFShmSetTest, STLCompatibility)
{
    // 从std::set构造
    std::set<int> std_set = {1, 3, 5, 7, 9};
    NFShmSet<int, 10> nf_set(std_set);
    EXPECT_EQ(nf_set.size(), std_set.size());

    // 从std::unordered_set构造
    std::unordered_set<int> unordered_set = {2, 4, 6, 8};
    NFShmSet<int, 10> nf_set2(unordered_set);
    EXPECT_EQ(nf_set2.size(), unordered_set.size());

    // 赋值操作符测试
    std::set<int> another_std_set = {10, 20, 30};
    nf_set = another_std_set;
    EXPECT_EQ(nf_set.size(), another_std_set.size());
    for (int val : another_std_set)
    {
        EXPECT_NE(nf_set.find(val), nf_set.end());
    }

    // 从std::unordered_set赋值
    std::unordered_set<int> another_unordered_set = {40, 50, 60};
    nf_set = another_unordered_set;
    EXPECT_EQ(nf_set.size(), another_unordered_set.size());
    for (int val : another_unordered_set)
    {
        EXPECT_NE(nf_set.find(val), nf_set.end());
    }
}

// 迭代器测试
TEST_F(NFShmSetTest, IteratorTests)
{
    NFShmSet<int, 10> set;
    std::vector<int> values = {5, 1, 9, 3, 7};

    for (int val : values)
    {
        set.insert(val);
    }

    // 正向迭代器测试
    std::vector<int> sorted_values = {1, 3, 5, 7, 9};
    std::vector<int> iterated_values;
    for (auto it = set.begin(); it != set.end(); ++it)
    {
        iterated_values.push_back(*it);
    }
    EXPECT_EQ(iterated_values, sorted_values);

    // 反向迭代器测试
    std::vector<int> reverse_sorted = {9, 7, 5, 3, 1};
    std::vector<int> reverse_iterated;
    for (auto it = set.rbegin(); it != set.rend(); ++it)
    {
        reverse_iterated.push_back(*it);
    }
    EXPECT_EQ(reverse_iterated, reverse_sorted);

    // const迭代器测试
    const NFShmSet<int, 10>& const_set = set;
    std::vector<int> const_iterated;
    for (auto it = const_set.cbegin(); it != const_set.cend(); ++it)
    {
        const_iterated.push_back(*it);
    }
    EXPECT_EQ(const_iterated, sorted_values);

    // 范围for循环测试
    std::vector<int> range_for_values;
    for (const auto& val : set)
    {
        range_for_values.push_back(val);
    }
    EXPECT_EQ(range_for_values, sorted_values);
}

// 边界条件测试
TEST_F(NFShmSetTest, BoundaryTests)
{
    NFShmSet<int, 3> small_set;

    // 测试容量限制
    small_set.insert(1);
    small_set.insert(2);
    small_set.insert(3);
    EXPECT_TRUE(small_set.full());
    EXPECT_EQ(small_set.size(), 3);

    // 尝试插入到满容器
    auto result = small_set.insert(4);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(small_set.size(), 3);

    // 测试清空
    small_set.clear();
    EXPECT_TRUE(small_set.empty());
    EXPECT_EQ(small_set.size(), 0);
    EXPECT_FALSE(small_set.full());

    // 重新插入
    small_set.insert(10);
    EXPECT_EQ(small_set.size(), 1);
    EXPECT_NE(small_set.find(10), small_set.end());
}

// emplace操作测试
TEST_F(NFShmSetTest, EmplaceOperations)
{
    NFShmSet<TestElement, 5> set;

    // 测试emplace
    auto result = set.emplace(1, "first");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->value, 1);
    EXPECT_EQ(result.first->name, "first");

    // 测试emplace重复元素
    result = set.emplace(1, "duplicate");
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->name, "first"); // 应该保持原值

    // 测试emplace_hint
    auto it = set.emplace_hint(set.end(), 2, std::string("second"));
    EXPECT_EQ(it->value, 2);
    EXPECT_EQ(it->name, "second");

    EXPECT_EQ(set.size(), 2);
}

// 批量操作测试
TEST_F(NFShmSetTest, BatchOperations)
{
    NFShmSet<int, 20> set;

    // 批量插入
    std::vector<int> values = {1, 3, 5, 7, 9, 11, 13, 15};
    set.insert(values.begin(), values.end());
    EXPECT_EQ(set.size(), values.size());

    // 批量删除
    std::vector<int> to_erase = {3, 7, 11, 15};
    for (int val : to_erase)
    {
        set.erase(val);
    }
    EXPECT_EQ(set.size(), values.size() - to_erase.size());

    // 验证剩余元素
    std::vector<int> remaining = {1, 5, 9, 13};
    for (int val : remaining)
    {
        EXPECT_NE(set.find(val), set.end());
    }
    for (int val : to_erase)
    {
        EXPECT_EQ(set.find(val), set.end());
    }
}

// 比较操作符测试
TEST_F(NFShmSetTest, ComparisonOperators)
{
    NFShmSet<int, 10> set1;
    NFShmSet<int, 10> set2;

    // 空容器比较
    EXPECT_TRUE(set1 == set2);
    EXPECT_FALSE(set1 != set2);
    EXPECT_FALSE(set1 < set2);
    EXPECT_TRUE(set1 <= set2);
    EXPECT_FALSE(set1 > set2);
    EXPECT_TRUE(set1 >= set2);

    // 添加相同元素
    set1.insert(1);
    set1.insert(2);
    set2.insert(1);
    set2.insert(2);
    EXPECT_TRUE(set1 == set2);
    EXPECT_FALSE(set1 != set2);

    // 添加不同元素
    set1.insert(3);
    set2.insert(4);
    EXPECT_FALSE(set1 == set2);
    EXPECT_TRUE(set1 != set2);
    EXPECT_TRUE(set1 < set2);
    EXPECT_TRUE(set1 <= set2);
    EXPECT_FALSE(set1 > set2);
    EXPECT_FALSE(set1 >= set2);
}

// 交换操作测试
TEST_F(NFShmSetTest, SwapOperations)
{
    NFShmSet<int, 10> set1;
    NFShmSet<int, 10> set2;

    // 准备测试数据
    set1.insert(1);
    set1.insert(3);
    set1.insert(5);

    set2.insert(2);
    set2.insert(4);

    size_t size1 = set1.size();
    size_t size2 = set2.size();

    // 成员函数swap
    set1.swap(set2);

    EXPECT_EQ(set1.size(), size2);
    EXPECT_EQ(set2.size(), size1);
    EXPECT_NE(set1.find(2), set1.end());
    EXPECT_NE(set1.find(4), set1.end());
    EXPECT_NE(set2.find(1), set2.end());
    EXPECT_NE(set2.find(3), set2.end());
    EXPECT_NE(set2.find(5), set2.end());

    // 全局swap函数
    swap(set1, set2);

    EXPECT_EQ(set1.size(), size1);
    EXPECT_EQ(set2.size(), size2);
    EXPECT_NE(set1.find(1), set1.end());
    EXPECT_NE(set1.find(3), set1.end());
    EXPECT_NE(set1.find(5), set1.end());
}

// 性能基准测试
TEST_F(NFShmSetTest, PerformanceBasics)
{
    const size_t LARGE_SIZE = 1000;
    NFShmSet<int, LARGE_SIZE> large_set;

    // 大量插入测试
    for (size_t i = 0; i < LARGE_SIZE / 2; ++i)
    {
        large_set.insert(static_cast<int>(i * 2));
    }
    EXPECT_EQ(large_set.size(), LARGE_SIZE / 2);

    // 大量查找测试
    for (size_t i = 0; i < LARGE_SIZE / 2; ++i)
    {
        EXPECT_NE(large_set.find(static_cast<int>(i * 2)), large_set.end());
        EXPECT_EQ(large_set.find(static_cast<int>(i * 2 + 1)), large_set.end());
    }

    // 大量删除测试
    for (size_t i = 0; i < LARGE_SIZE / 4; ++i)
    {
        large_set.erase(static_cast<int>(i * 2));
    }
    EXPECT_EQ(large_set.size(), LARGE_SIZE / 2 - LARGE_SIZE / 4);
}
