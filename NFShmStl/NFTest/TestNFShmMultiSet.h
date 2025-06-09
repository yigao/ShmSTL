// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmMultiSet.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmMultiSet
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmMultiSet.h"
#include <string>
#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>

// 重用MultiTestElement类定义
class MultiTestElement
{
public:
    MultiTestElement() : value(0), name("default")
    {
        ++constructor_count;
    }

    MultiTestElement(int v) : value(v), name("value_" + std::to_string(v))
    {
        ++constructor_count;
    }

    MultiTestElement(int v, const std::string& n) : value(v), name(n)
    {
        ++constructor_count;
    }

    MultiTestElement(const MultiTestElement& other) : value(other.value), name(other.name)
    {
        ++constructor_count;
    }

    MultiTestElement& operator=(const MultiTestElement& other)
    {
        if (this != &other)
        {
            value = other.value;
            name = other.name;
        }
        return *this;
    }

    ~MultiTestElement()
    {
        ++destructor_count;
    }

    bool operator==(const MultiTestElement& other) const
    {
        return value == other.value;
    }

    bool operator<(const MultiTestElement& other) const
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

int MultiTestElement::constructor_count = 0;
int MultiTestElement::destructor_count = 0;

class NFShmMultiSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MultiTestElement::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(MultiTestElement::constructor_count, MultiTestElement::destructor_count);
    }
};

// 基本功能测试 - 允许重复元素
TEST_F(NFShmMultiSetTest, BasicOperationsWithDuplicates)
{
    NFShmMultiSet<int, 10> mset;

    // 测试空容器
    EXPECT_TRUE(mset.empty());
    EXPECT_EQ(mset.size(), 0);
    EXPECT_EQ(mset.max_size(), 10);
    EXPECT_FALSE(mset.full());

    // 测试插入重复元素
    auto it1 = mset.insert(5);
    auto it2 = mset.insert(5);
    auto it3 = mset.insert(5);

    EXPECT_EQ(*it1, 5);
    EXPECT_EQ(*it2, 5);
    EXPECT_EQ(*it3, 5);
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(5), 3);

    // 测试插入不同元素
    mset.insert(3);
    mset.insert(7);
    mset.insert(3);
    EXPECT_EQ(mset.size(), 6);
    EXPECT_EQ(mset.count(3), 2);
    EXPECT_EQ(mset.count(7), 1);

    // 测试查找
    auto it = mset.find(5);
    EXPECT_NE(it, mset.end());
    EXPECT_EQ(*it, 5);

    it = mset.find(10);
    EXPECT_EQ(it, mset.end());

    // 测试lower_bound和upper_bound
    auto lower = mset.lower_bound(5);
    auto upper = mset.upper_bound(5);
    EXPECT_NE(lower, mset.end());
    EXPECT_EQ(*lower, 5);
    EXPECT_EQ(std::distance(lower, upper), 3); // 有3个5

    // 测试equal_range
    auto range = mset.equal_range(5);
    EXPECT_EQ(range.first, lower);
    EXPECT_EQ(range.second, upper);
    EXPECT_EQ(std::distance(range.first, range.second), 3);

    // 测试删除所有相同元素
    size_t erased = mset.erase(5);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(5), 0);
    EXPECT_EQ(mset.find(5), mset.end());

    // 验证其他元素仍存在
    EXPECT_EQ(mset.count(3), 2);
    EXPECT_EQ(mset.count(7), 1);
}

// 字符串类型重复测试
TEST_F(NFShmMultiSetTest, StringDuplicateOperations)
{
    NFShmMultiSet<std::string, 8> mset;

    // 插入重复字符串
    mset.insert("hello");
    mset.insert("world");
    mset.insert("hello");
    mset.insert("test");
    mset.insert("hello");

    EXPECT_EQ(mset.size(), 5);
    EXPECT_EQ(mset.count("hello"), 3);
    EXPECT_EQ(mset.count("world"), 1);
    EXPECT_EQ(mset.count("test"), 1);

    // 测试排序（相同元素应该相邻）
    std::vector<std::string> expected = {"hello", "hello", "hello", "test", "world"};
    std::vector<std::string> actual;
    for (const auto& str : mset)
    {
        actual.push_back(str);
    }
    EXPECT_EQ(actual, expected);

    // 测试range操作
    auto range = mset.equal_range("hello");
    EXPECT_EQ(std::distance(range.first, range.second), 3);

    // 验证range中所有元素都是"hello"
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(*it, "hello");
    }
}

// 自定义类型重复测试
TEST_F(NFShmMultiSetTest, CustomTypeDuplicateOperations)
{
    NFShmMultiSet<MultiTestElement, 10> mset;

    // 插入具有相同value但不同name的元素
    MultiTestElement elem1(1, "first");
    MultiTestElement elem2(1, "second");
    MultiTestElement elem3(2, "third");
    MultiTestElement elem4(1, "fourth");

    mset.insert(elem1);
    mset.insert(elem2);
    mset.insert(elem3);
    mset.insert(elem4);

    EXPECT_EQ(mset.size(), 4);
    EXPECT_EQ(mset.count(MultiTestElement(1)), 3); // value为1的有3个
    EXPECT_EQ(mset.count(MultiTestElement(2)), 1); // value为2的有1个

    // 测试排序和查找
    std::vector<int> expected_values = {1, 1, 1, 2};
    std::vector<int> actual_values;
    for (const auto& elem : mset)
    {
        actual_values.push_back(elem.value);
    }
    EXPECT_EQ(actual_values, expected_values);

    // 测试equal_range
    auto range = mset.equal_range(MultiTestElement(1));
    EXPECT_EQ(std::distance(range.first, range.second), 3);

    // 验证range中所有元素的value都是1，但name不同
    std::vector<std::string> names;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->value, 1);
        names.push_back(it->name);
    }
    EXPECT_EQ(names.size(), 3);
    // 验证所有name都存在
    EXPECT_NE(std::find(names.begin(), names.end(), "first"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "second"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "fourth"), names.end());
}

// 构造函数测试
TEST_F(NFShmMultiSetTest, ConstructorTests)
{
    // 默认构造函数
    NFShmMultiSet<int, 5> mset1;
    EXPECT_TRUE(mset1.empty());

    // 范围构造函数（包含重复元素）
    std::vector<int> vec = {1, 3, 1, 5, 3, 7, 1};
    NFShmMultiSet<int, 15> mset2(vec.begin(), vec.end());
    EXPECT_EQ(mset2.size(), 7);
    EXPECT_EQ(mset2.count(1), 3);
    EXPECT_EQ(mset2.count(3), 2);
    EXPECT_EQ(mset2.count(5), 1);
    EXPECT_EQ(mset2.count(7), 1);

    // 拷贝构造函数
    NFShmMultiSet<int, 15> mset3(mset2);
    EXPECT_EQ(mset3.size(), mset2.size());
    EXPECT_TRUE(std::equal(mset2.begin(), mset2.end(), mset3.begin()));

    // 指针范围构造函数（包含重复元素）
    int arr[] = {2, 4, 2, 6, 4, 8, 2};
    NFShmMultiSet<int, 15> mset4(arr, arr + 7);
    EXPECT_EQ(mset4.size(), 7);
    EXPECT_EQ(mset4.count(2), 3);
    EXPECT_EQ(mset4.count(4), 2);
    EXPECT_EQ(mset4.count(6), 1);
    EXPECT_EQ(mset4.count(8), 1);

    // const_iterator范围构造函数
    NFShmMultiSet<int, 15> mset5(mset2.begin(), mset2.end());
    EXPECT_EQ(mset5.size(), mset2.size());
    EXPECT_TRUE(std::equal(mset2.begin(), mset2.end(), mset5.begin()));
}

// STL兼容性测试
TEST_F(NFShmMultiSetTest, STLCompatibility)
{
    // 从std::multiset构造
    std::multiset<int> std_mset = {1, 3, 1, 5, 3, 7, 1};
    NFShmMultiSet<int, 15> nf_mset(std_mset);
    EXPECT_EQ(nf_mset.size(), std_mset.size());
    EXPECT_EQ(nf_mset.count(1), std_mset.count(1));
    EXPECT_EQ(nf_mset.count(3), std_mset.count(3));

    // 从std::set构造
    std::set<int> std_set = {2, 4, 6, 8};
    NFShmMultiSet<int, 15> nf_mset2(std_set);
    EXPECT_EQ(nf_mset2.size(), std_set.size());

    // 从std::unordered_set构造
    std::unordered_set<int> unordered_set = {10, 20, 30};
    NFShmMultiSet<int, 15> nf_mset3(unordered_set);
    EXPECT_EQ(nf_mset3.size(), unordered_set.size());

    // 赋值操作符测试
    std::multiset<int> another_std_mset = {100, 200, 100, 300, 200, 100};
    nf_mset = another_std_mset;
    EXPECT_EQ(nf_mset.size(), another_std_mset.size());
    EXPECT_EQ(nf_mset.count(100), another_std_mset.count(100));
    EXPECT_EQ(nf_mset.count(200), another_std_mset.count(200));
    EXPECT_EQ(nf_mset.count(300), another_std_mset.count(300));

    // 从std::set赋值
    std::set<int> another_std_set = {40, 50, 60};
    nf_mset = another_std_set;
    EXPECT_EQ(nf_mset.size(), another_std_set.size());
    for (int val : another_std_set)
    {
        EXPECT_EQ(nf_mset.count(val), 1);
    }
}

// 迭代器测试
TEST_F(NFShmMultiSetTest, IteratorTests)
{
    NFShmMultiSet<int, 15> mset;
    std::vector<int> values = {5, 1, 9, 3, 7, 5, 3, 1, 5};

    for (int val : values)
    {
        mset.insert(val);
    }

    // 正向迭代器测试（排序后的结果）
    std::vector<int> sorted_values = {1, 1, 3, 3, 5, 5, 5, 7, 9};
    std::vector<int> iterated_values;
    for (auto it = mset.begin(); it != mset.end(); ++it)
    {
        iterated_values.push_back(*it);
    }
    EXPECT_EQ(iterated_values, sorted_values);

    // 反向迭代器测试
    std::vector<int> reverse_sorted = {9, 7, 5, 5, 5, 3, 3, 1, 1};
    std::vector<int> reverse_iterated;
    for (auto it = mset.rbegin(); it != mset.rend(); ++it)
    {
        reverse_iterated.push_back(*it);
    }
    EXPECT_EQ(reverse_iterated, reverse_sorted);

    // const迭代器测试
    const NFShmMultiSet<int, 15>& const_mset = mset;
    std::vector<int> const_iterated;
    for (auto it = const_mset.cbegin(); it != const_mset.cend(); ++it)
    {
        const_iterated.push_back(*it);
    }
    EXPECT_EQ(const_iterated, sorted_values);

    // 范围for循环测试
    std::vector<int> range_for_values;
    for (const auto& val : mset)
    {
        range_for_values.push_back(val);
    }
    EXPECT_EQ(range_for_values, sorted_values);
}

// 边界条件测试
TEST_F(NFShmMultiSetTest, BoundaryTests)
{
    NFShmMultiSet<int, 5> small_mset;

    // 测试容量限制
    small_mset.insert(1);
    small_mset.insert(1);
    small_mset.insert(1);
    small_mset.insert(2);
    small_mset.insert(2);
    EXPECT_TRUE(small_mset.full());
    EXPECT_EQ(small_mset.size(), 5);
    EXPECT_EQ(small_mset.count(1), 3);
    EXPECT_EQ(small_mset.count(2), 2);

    // 尝试插入到满容器
    auto it = small_mset.insert(3);
    EXPECT_EQ(it, small_mset.end()); // 插入失败
    EXPECT_EQ(small_mset.size(), 5);

    // 测试清空
    small_mset.clear();
    EXPECT_TRUE(small_mset.empty());
    EXPECT_EQ(small_mset.size(), 0);
    EXPECT_FALSE(small_mset.full());

    // 重新插入
    small_mset.insert(10);
    small_mset.insert(10);
    EXPECT_EQ(small_mset.size(), 2);
    EXPECT_EQ(small_mset.count(10), 2);
}

// emplace操作测试
TEST_F(NFShmMultiSetTest, EmplaceOperations)
{
    NFShmMultiSet<MultiTestElement, 8> mset;

    // 测试emplace重复元素
    auto it1 = mset.emplace(1, "first");
    auto it2 = mset.emplace(1, "second");
    auto it3 = mset.emplace(1, "third");

    EXPECT_EQ(it1->value, 1);
    EXPECT_EQ(it2->value, 1);
    EXPECT_EQ(it3->value, 1);
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(MultiTestElement(1)), 3);

    // 测试emplace_hint
    auto it4 = mset.emplace_hint(mset.end(), 2, "fourth");
    EXPECT_EQ(it4->value, 2);
    EXPECT_EQ(it4->name, "fourth");

    EXPECT_EQ(mset.size(), 4);
}

// 删除操作测试
TEST_F(NFShmMultiSetTest, EraseOperations)
{
    NFShmMultiSet<int, 15> mset;

    // 准备测试数据
    std::vector<int> values = {1, 3, 1, 5, 3, 7, 1, 9, 3};
    for (int val : values)
    {
        mset.insert(val);
    }

    EXPECT_EQ(mset.size(), 9);
    EXPECT_EQ(mset.count(1), 3);
    EXPECT_EQ(mset.count(3), 3);
    EXPECT_EQ(mset.count(5), 1);
    EXPECT_EQ(mset.count(7), 1);
    EXPECT_EQ(mset.count(9), 1);

    // 测试删除单个迭代器
    auto it = mset.find(1);
    EXPECT_NE(it, mset.end());
    mset.erase(it);
    EXPECT_EQ(mset.size(), 8);
    EXPECT_EQ(mset.count(1), 2); // 只删除一个1

    // 测试删除所有相同键的元素
    size_t erased = mset.erase(3);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(mset.size(), 5);
    EXPECT_EQ(mset.count(3), 0);

    // 测试范围删除
    auto range = mset.equal_range(1);
    mset.erase(range.first, range.second);
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(1), 0);

    // 验证剩余元素
    EXPECT_EQ(mset.count(5), 1);
    EXPECT_EQ(mset.count(7), 1);
    EXPECT_EQ(mset.count(9), 1);
}

// 批量操作测试
TEST_F(NFShmMultiSetTest, BatchOperations)
{
    NFShmMultiSet<int, 30> mset;

    // 批量插入包含重复元素
    std::vector<int> values = {1, 3, 5, 1, 7, 3, 9, 1, 11, 5, 13, 3, 15};
    mset.insert(values.begin(), values.end());
    EXPECT_EQ(mset.size(), values.size());
    EXPECT_EQ(mset.count(1), 3);
    EXPECT_EQ(mset.count(3), 3);
    EXPECT_EQ(mset.count(5), 2);

    // 批量删除
    std::vector<int> to_erase = {1, 5, 11};
    for (int val : to_erase)
    {
        mset.erase(val);
    }

    // 验证删除结果
    EXPECT_EQ(mset.count(1), 0);
    EXPECT_EQ(mset.count(5), 0);
    EXPECT_EQ(mset.count(11), 0);
    EXPECT_EQ(mset.count(3), 3); // 仍有3个
    EXPECT_EQ(mset.count(7), 1);
    EXPECT_EQ(mset.count(9), 1);
    EXPECT_EQ(mset.count(13), 1);
    EXPECT_EQ(mset.count(15), 1);
}

// 比较操作符测试
TEST_F(NFShmMultiSetTest, ComparisonOperators)
{
    NFShmMultiSet<int, 10> mset1;
    NFShmMultiSet<int, 10> mset2;

    // 空容器比较
    EXPECT_TRUE(mset1 == mset2);
    EXPECT_FALSE(mset1 != mset2);

    // 添加相同元素
    mset1.insert(1);
    mset1.insert(1);
    mset1.insert(2);

    mset2.insert(1);
    mset2.insert(1);
    mset2.insert(2);

    EXPECT_TRUE(mset1 == mset2);
    EXPECT_FALSE(mset1 != mset2);

    // 添加不同数量的相同元素
    mset1.insert(3);
    mset2.insert(3);
    mset2.insert(3);

    EXPECT_FALSE(mset1 == mset2);
    EXPECT_TRUE(mset1 != mset2);
    EXPECT_TRUE(mset1 < mset2);
    EXPECT_FALSE(mset1 > mset2);
}

// 交换操作测试
TEST_F(NFShmMultiSetTest, SwapOperations)
{
    NFShmMultiSet<int, 15> mset1;
    NFShmMultiSet<int, 15> mset2;

    // 准备测试数据
    mset1.insert(1);
    mset1.insert(1);
    mset1.insert(3);
    mset1.insert(5);

    mset2.insert(2);
    mset2.insert(4);
    mset2.insert(2);

    size_t size1 = mset1.size();
    size_t size2 = mset2.size();

    // 成员函数swap
    mset1.swap(mset2);

    EXPECT_EQ(mset1.size(), size2);
    EXPECT_EQ(mset2.size(), size1);
    EXPECT_EQ(mset1.count(2), 2);
    EXPECT_EQ(mset1.count(4), 1);
    EXPECT_EQ(mset2.count(1), 2);
    EXPECT_EQ(mset2.count(3), 1);
    EXPECT_EQ(mset2.count(5), 1);

    // 全局swap函数
    swap(mset1, mset2);

    EXPECT_EQ(mset1.size(), size1);
    EXPECT_EQ(mset2.size(), size2);
    EXPECT_EQ(mset1.count(1), 2);
    EXPECT_EQ(mset1.count(3), 1);
    EXPECT_EQ(mset1.count(5), 1);
}

// 性能基准测试
TEST_F(NFShmMultiSetTest, PerformanceBasics)
{
    const size_t LARGE_SIZE = 1000;
    NFShmMultiSet<int, LARGE_SIZE> large_mset;

    // 大量插入测试（包含重复）
    for (size_t i = 0; i < LARGE_SIZE / 3; ++i)
    {
        large_mset.insert(static_cast<int>(i % 100)); // 创建重复元素
    }
    EXPECT_EQ(large_mset.size(), LARGE_SIZE / 3);

    // 验证重复元素计数
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_GT(large_mset.count(i), 0);
    }

    // 大量查找测试
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_NE(large_mset.find(i), large_mset.end());
    }
    EXPECT_EQ(large_mset.find(1000), large_mset.end());

    // 大量删除测试
    for (int i = 0; i < 50; ++i)
    {
        size_t erased = large_mset.erase(i);
        EXPECT_GT(erased, 0);
        EXPECT_EQ(large_mset.count(i), 0);
    }
}
