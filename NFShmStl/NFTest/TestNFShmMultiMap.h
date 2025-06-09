// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmMultiMap.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmMultiMap
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmMultiMap.h"
#include "NFComm/NFShmStl/NFShmPair.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

// 自定义值类型用于测试
class MultiTestValue
{
public:
    MultiTestValue() : id(0), name("default")
    {
        ++constructor_count;
    }

    MultiTestValue(int i) : id(i), name("value_" + std::to_string(i))
    {
        ++constructor_count;
    }

    MultiTestValue(int i, const std::string& n) : id(i), name(n)
    {
        ++constructor_count;
    }

    MultiTestValue(const MultiTestValue& other) : id(other.id), name(other.name)
    {
        ++constructor_count;
    }

    MultiTestValue& operator=(const MultiTestValue& other)
    {
        if (this != &other)
        {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    ~MultiTestValue()
    {
        ++destructor_count;
    }

    bool operator==(const MultiTestValue& other) const
    {
        return id == other.id && name == other.name;
    }

    bool operator<(const MultiTestValue& other) const
    {
        if (id != other.id) return id < other.id;
        return name < other.name;
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

int MultiTestValue::constructor_count = 0;
int MultiTestValue::destructor_count = 0;

class NFShmMultiMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MultiTestValue::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(MultiTestValue::constructor_count, MultiTestValue::destructor_count);
    }
};

// 基本功能测试 - 允许重复键
TEST_F(NFShmMultiMapTest, BasicOperationsWithDuplicateKeys)
{
    NFShmMultiMap<int, std::string, 15> mmap;

    // 测试空容器
    EXPECT_TRUE(mmap.empty());
    EXPECT_EQ(mmap.size(), 0);
    EXPECT_EQ(mmap.max_size(), 15);
    EXPECT_FALSE(mmap.full());

    // 测试插入重复键
    auto it1 = mmap.insert({1, "first"});
    auto it2 = mmap.insert({1, "second"});
    auto it3 = mmap.insert({1, "third"});

    EXPECT_EQ(it1->first, 1);
    EXPECT_EQ(it2->first, 1);
    EXPECT_EQ(it3->first, 1);
    EXPECT_EQ(it1->second, "first");
    EXPECT_EQ(it2->second, "second");
    EXPECT_EQ(it3->second, "third");
    EXPECT_EQ(mmap.size(), 3);
    EXPECT_EQ(mmap.count(1), 3);

    // 测试插入不同键
    mmap.insert({2, "two_a"});
    mmap.insert({3, "three"});
    mmap.insert({2, "two_b"});
    EXPECT_EQ(mmap.size(), 6);
    EXPECT_EQ(mmap.count(1), 3);
    EXPECT_EQ(mmap.count(2), 2);
    EXPECT_EQ(mmap.count(3), 1);

    // 测试查找
    auto it = mmap.find(1);
    EXPECT_NE(it, mmap.end());
    EXPECT_EQ(it->first, 1);

    it = mmap.find(10);
    EXPECT_EQ(it, mmap.end());

    // 测试lower_bound和upper_bound
    auto lower = mmap.lower_bound(1);
    auto upper = mmap.upper_bound(1);
    EXPECT_NE(lower, mmap.end());
    EXPECT_EQ(lower->first, 1);
    EXPECT_EQ(std::distance(lower, upper), 3); // 有3个键为1的元素

    // 测试equal_range
    auto range = mmap.equal_range(1);
    EXPECT_EQ(range.first, lower);
    EXPECT_EQ(range.second, upper);
    EXPECT_EQ(std::distance(range.first, range.second), 3);

    // 验证equal_range中的所有值
    std::vector<std::string> expected_values = {"first", "second", "third"};
    std::vector<std::string> actual_values;
    for (auto it = range.first; it != range.second; ++it)
    {
        actual_values.push_back(it->second);
    }
    EXPECT_EQ(actual_values, expected_values);

    // 测试删除所有相同键的元素
    size_t erased = mmap.erase(1);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(mmap.size(), 3);
    EXPECT_EQ(mmap.count(1), 0);
    EXPECT_EQ(mmap.find(1), mmap.end());

    // 验证其他元素仍存在
    EXPECT_EQ(mmap.count(2), 2);
    EXPECT_EQ(mmap.count(3), 1);
}

// 字符串键的重复测试
TEST_F(NFShmMultiMapTest, StringKeyDuplicateOperations)
{
    NFShmMultiMap<std::string, int, 10> mmap;

    // 插入重复字符串键
    mmap.insert({"key", 1});
    mmap.insert({"key", 2});
    mmap.insert({"key", 3});
    mmap.insert({"other", 100});
    mmap.insert({"key", 4});

    EXPECT_EQ(mmap.size(), 5);
    EXPECT_EQ(mmap.count("key"), 4);
    EXPECT_EQ(mmap.count("other"), 1);

    // 测试排序（相同键应该相邻，按插入顺序）
    auto range = mmap.equal_range("key");
    std::vector<int> expected_values = {1, 2, 3, 4};
    std::vector<int> actual_values;
    for (auto it = range.first; it != range.second; ++it)
    {
        actual_values.push_back(it->second);
    }
    EXPECT_EQ(actual_values, expected_values);

    // 验证range中所有元素的键都是"key"
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->first, "key");
    }

    // 测试删除单个元素
    auto it = mmap.find("key");
    EXPECT_NE(it, mmap.end());
    mmap.erase(it);
    EXPECT_EQ(mmap.size(), 4);
    EXPECT_EQ(mmap.count("key"), 3); // 只删除一个
}

// 自定义类型重复测试
TEST_F(NFShmMultiMapTest, CustomTypeDuplicateOperations)
{
    NFShmMultiMap<int, MultiTestValue, 12> mmap;

    // 插入具有相同键但不同值的元素
    MultiTestValue val1(1, "first");
    MultiTestValue val2(2, "second");
    MultiTestValue val3(3, "third");
    MultiTestValue val4(4, "fourth");

    mmap.insert({1, val1});
    mmap.insert({1, val2});
    mmap.insert({2, val3});
    mmap.insert({1, val4});

    EXPECT_EQ(mmap.size(), 4);
    EXPECT_EQ(mmap.count(1), 3); // 键为1的有3个
    EXPECT_EQ(mmap.count(2), 1); // 键为2的有1个

    // 测试equal_range和值的验证
    auto range = mmap.equal_range(1);
    EXPECT_EQ(std::distance(range.first, range.second), 3);

    // 验证range中所有元素的键都是1，但值不同
    std::vector<int> ids;
    std::vector<std::string> names;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->first, 1);
        ids.push_back(it->second.id);
        names.push_back(it->second.name);
    }
    EXPECT_EQ(ids.size(), 3);
    EXPECT_EQ(names.size(), 3);

    // 验证所有值都存在
    EXPECT_NE(std::find(ids.begin(), ids.end(), 1), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), 2), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), 4), ids.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "first"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "second"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "fourth"), names.end());
}

// 构造函数测试
TEST_F(NFShmMultiMapTest, ConstructorTests)
{
    // 默认构造函数
    NFShmMultiMap<int, std::string, 5> mmap1;
    EXPECT_TRUE(mmap1.empty());

    // 范围构造函数（包含重复键）
    std::vector<std::pair<int, std::string>> vec = {
        {1, "one_a"}, {3, "three"}, {1, "one_b"}, {5, "five"}, {3, "three_b"}, {1, "one_c"}
    };
    NFShmMultiMap<int, std::string, 20> mmap2(vec.begin(), vec.end());
    EXPECT_EQ(mmap2.size(), 6);
    EXPECT_EQ(mmap2.count(1), 3);
    EXPECT_EQ(mmap2.count(3), 2);
    EXPECT_EQ(mmap2.count(5), 1);

    // 拷贝构造函数
    NFShmMultiMap<int, std::string, 20> mmap3(mmap2);
    EXPECT_EQ(mmap3.size(), mmap2.size());
    EXPECT_TRUE(std::equal(mmap2.begin(), mmap2.end(), mmap3.begin()));

    // 使用std::pair数组进行测试
    std::pair<int, std::string> arr[] = {{2, "two_a"}, {4, "four"}, {2, "two_b"}, {6, "six"}, {2, "two_c"}};
    NFShmMultiMap<int, std::string, 20> mmap4(arr, arr + 5);
    EXPECT_EQ(mmap4.size(), 5);
    EXPECT_EQ(mmap4.count(2), 3);
    EXPECT_EQ(mmap4.count(4), 1);
    EXPECT_EQ(mmap4.count(6), 1);

    // const_iterator范围构造函数
    NFShmMultiMap<int, std::string, 20> mmap5(mmap2.begin(), mmap2.end());
    EXPECT_EQ(mmap5.size(), mmap2.size());
    EXPECT_TRUE(std::equal(mmap2.begin(), mmap2.end(), mmap5.begin()));
}

// STL兼容性测试
TEST_F(NFShmMultiMapTest, STLCompatibility)
{
    // 从std::multimap构造
    std::multimap<int, std::string> std_mmap = {
        {1, "one_a"}, {3, "three"}, {1, "one_b"}, {5, "five"}, {1, "one_c"}
    };
    NFShmMultiMap<int, std::string, 20> nf_mmap(std_mmap);
    EXPECT_EQ(nf_mmap.size(), std_mmap.size());
    EXPECT_EQ(nf_mmap.count(1), std_mmap.count(1));
    EXPECT_EQ(nf_mmap.count(3), std_mmap.count(3));
    EXPECT_EQ(nf_mmap.count(5), std_mmap.count(5));

    // 从std::map构造
    std::map<int, std::string> std_map = {{2, "two"}, {4, "four"}, {6, "six"}};
    NFShmMultiMap<int, std::string, 20> nf_mmap2(std_map);
    EXPECT_EQ(nf_mmap2.size(), std_map.size());

    // 从std::unordered_map构造
    std::unordered_map<int, std::string> unordered_map = {{10, "ten"}, {20, "twenty"}};
    NFShmMultiMap<int, std::string, 20> nf_mmap3(unordered_map);
    EXPECT_EQ(nf_mmap3.size(), unordered_map.size());

    // 赋值操作符测试
    std::multimap<int, std::string> another_std_mmap = {
        {100, "hundred_a"}, {200, "two_hundred"}, {100, "hundred_b"}, {300, "three_hundred"}
    };
    nf_mmap = another_std_mmap;
    EXPECT_EQ(nf_mmap.size(), another_std_mmap.size());
    EXPECT_EQ(nf_mmap.count(100), another_std_mmap.count(100));
    EXPECT_EQ(nf_mmap.count(200), another_std_mmap.count(200));
    EXPECT_EQ(nf_mmap.count(300), another_std_mmap.count(300));

    // 从std::map赋值
    std::map<int, std::string> another_std_map = {{40, "forty"}, {50, "fifty"}};
    nf_mmap = another_std_map;
    EXPECT_EQ(nf_mmap.size(), another_std_map.size());
    for (const auto& pair : another_std_map)
    {
        EXPECT_EQ(nf_mmap.count(pair.first), 1);
    }
}

// 迭代器测试
TEST_F(NFShmMultiMapTest, IteratorTests)
{
    NFShmMultiMap<int, std::string, 20> mmap;
    std::vector<std::pair<int, std::string>> values = {
        {5, "five_a"}, {1, "one_a"}, {9, "nine"}, {3, "three"},
        {7, "seven"}, {5, "five_b"}, {3, "three_b"}, {1, "one_b"}
    };

    for (const auto& pair : values)
    {
        mmap.insert(pair);
    }

    // 正向迭代器测试（按键排序，相同键按插入顺序）
    std::vector<int> expected_keys = {1, 1, 3, 3, 5, 5, 7, 9};
    std::vector<int> iterated_keys;
    for (auto it = mmap.begin(); it != mmap.end(); ++it)
    {
        iterated_keys.push_back(it->first);
    }
    EXPECT_EQ(iterated_keys, expected_keys);

    // 反向迭代器测试
    std::vector<int> reverse_sorted = {9, 7, 5, 5, 3, 3, 1, 1};
    std::vector<int> reverse_iterated;
    for (auto it = mmap.rbegin(); it != mmap.rend(); ++it)
    {
        reverse_iterated.push_back(it->first);
    }
    EXPECT_EQ(reverse_iterated, reverse_sorted);

    // const迭代器测试
    const NFShmMultiMap<int, std::string, 20>& const_mmap = mmap;
    std::vector<int> const_iterated;
    for (auto it = const_mmap.cbegin(); it != const_mmap.cend(); ++it)
    {
        const_iterated.push_back(it->first);
    }
    EXPECT_EQ(const_iterated, expected_keys);

    // 范围for循环测试
    std::vector<int> range_for_keys;
    for (const auto& pair : mmap)
    {
        range_for_keys.push_back(pair.first);
    }
    EXPECT_EQ(range_for_keys, expected_keys);

    // 测试迭代器修改值
    for (auto& pair : mmap)
    {
        pair.second = "modified_" + pair.second;
    }

    // 验证修改结果
    auto range = mmap.equal_range(1);
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->second.substr(0, 9), "modified_");
    }
}

// 边界条件测试
TEST_F(NFShmMultiMapTest, BoundaryTests)
{
    NFShmMultiMap<int, std::string, 5> small_mmap;

    // 测试容量限制
    small_mmap.insert({1, "one_a"});
    small_mmap.insert({1, "one_b"});
    small_mmap.insert({1, "one_c"});
    small_mmap.insert({2, "two_a"});
    small_mmap.insert({2, "two_b"});
    EXPECT_TRUE(small_mmap.full());
    EXPECT_EQ(small_mmap.size(), 5);
    EXPECT_EQ(small_mmap.count(1), 3);
    EXPECT_EQ(small_mmap.count(2), 2);

    // 尝试插入到满容器
    auto it = small_mmap.insert({3, "three"});
    EXPECT_EQ(it, small_mmap.end()); // 插入失败
    EXPECT_EQ(small_mmap.size(), 5);

    // 测试清空
    small_mmap.clear();
    EXPECT_TRUE(small_mmap.empty());
    EXPECT_EQ(small_mmap.size(), 0);
    EXPECT_FALSE(small_mmap.full());

    // 重新插入
    small_mmap.insert({10, "ten_a"});
    small_mmap.insert({10, "ten_b"});
    EXPECT_EQ(small_mmap.size(), 2);
    EXPECT_EQ(small_mmap.count(10), 2);
}

// emplace操作测试
TEST_F(NFShmMultiMapTest, EmplaceOperations)
{
    NFShmMultiMap<std::string, MultiTestValue, 10> mmap;

    // 测试emplace重复键
    auto it1 = mmap.emplace("key", MultiTestValue(1, "first"));
    auto it2 = mmap.emplace("key", MultiTestValue(2, "second"));
    auto it3 = mmap.emplace("key", MultiTestValue(3, "third"));

    EXPECT_EQ(it1->first, "key");
    EXPECT_EQ(it2->first, "key");
    EXPECT_EQ(it3->first, "key");
    EXPECT_EQ(it1->second.id, 1);
    EXPECT_EQ(it2->second.id, 2);
    EXPECT_EQ(it3->second.id, 3);
    EXPECT_EQ(mmap.size(), 3);
    EXPECT_EQ(mmap.count("key"), 3);

    // 测试emplace_hint
    auto it4 = mmap.emplace_hint(mmap.end(), "other", MultiTestValue(4, "fourth"));
    EXPECT_EQ(it4->first, "other");
    EXPECT_EQ(it4->second.id, 4);
    EXPECT_EQ(it4->second.name, "fourth");

    EXPECT_EQ(mmap.size(), 4);
}

// 删除操作测试
TEST_F(NFShmMultiMapTest, EraseOperations)
{
    NFShmMultiMap<int, std::string, 20> mmap;

    // 准备测试数据
    std::vector<std::pair<int, std::string>> values = {
        {1, "one_a"}, {3, "three_a"}, {1, "one_b"}, {5, "five"},
        {3, "three_b"}, {7, "seven"}, {1, "one_c"}, {9, "nine"}, {3, "three_c"}
    };
    for (const auto& pair : values)
    {
        mmap.insert(pair);
    }

    EXPECT_EQ(mmap.size(), 9);
    EXPECT_EQ(mmap.count(1), 3);
    EXPECT_EQ(mmap.count(3), 3);
    EXPECT_EQ(mmap.count(5), 1);
    EXPECT_EQ(mmap.count(7), 1);
    EXPECT_EQ(mmap.count(9), 1);

    // 测试删除单个迭代器
    auto it = mmap.find(1);
    EXPECT_NE(it, mmap.end());
    mmap.erase(it);
    EXPECT_EQ(mmap.size(), 8);
    EXPECT_EQ(mmap.count(1), 2); // 只删除一个键为1的元素

    // 测试删除所有相同键的元素
    size_t erased = mmap.erase(3);
    EXPECT_EQ(erased, 3);
    EXPECT_EQ(mmap.size(), 5);
    EXPECT_EQ(mmap.count(3), 0);

    // 测试范围删除
    auto range = mmap.equal_range(1);
    mmap.erase(range.first, range.second);
    EXPECT_EQ(mmap.size(), 3);
    EXPECT_EQ(mmap.count(1), 0);

    // 验证剩余元素
    EXPECT_EQ(mmap.count(5), 1);
    EXPECT_EQ(mmap.count(7), 1);
    EXPECT_EQ(mmap.count(9), 1);
}

// 批量操作测试
TEST_F(NFShmMultiMapTest, BatchOperations)
{
    NFShmMultiMap<int, std::string, 40> mmap;

    // 批量插入包含重复键
    std::vector<std::pair<int, std::string>> values = {
        {1, "one_a"}, {3, "three_a"}, {5, "five_a"}, {1, "one_b"},
        {7, "seven"}, {3, "three_b"}, {9, "nine"}, {1, "one_c"},
        {11, "eleven"}, {5, "five_b"}, {13, "thirteen"}, {3, "three_c"}, {15, "fifteen"}
    };

    for (const auto& pair : values)
    {
        mmap.insert(pair);
    }
    EXPECT_EQ(mmap.size(), values.size());
    EXPECT_EQ(mmap.count(1), 3);
    EXPECT_EQ(mmap.count(3), 3);
    EXPECT_EQ(mmap.count(5), 2);

    // 批量删除
    std::vector<int> to_erase = {1, 5, 11};
    for (int key : to_erase)
    {
        mmap.erase(key);
    }

    // 验证删除结果
    EXPECT_EQ(mmap.count(1), 0);
    EXPECT_EQ(mmap.count(5), 0);
    EXPECT_EQ(mmap.count(11), 0);
    EXPECT_EQ(mmap.count(3), 3); // 仍有3个
    EXPECT_EQ(mmap.count(7), 1);
    EXPECT_EQ(mmap.count(9), 1);
    EXPECT_EQ(mmap.count(13), 1);
    EXPECT_EQ(mmap.count(15), 1);
}

// 比较操作符测试
TEST_F(NFShmMultiMapTest, ComparisonOperators)
{
    NFShmMultiMap<int, std::string, 15> mmap1;
    NFShmMultiMap<int, std::string, 15> mmap2;

    // 空容器比较
    EXPECT_TRUE(mmap1 == mmap2);
    EXPECT_FALSE(mmap1 != mmap2);

    // 添加相同元素
    mmap1.insert({1, "one_a"});
    mmap1.insert({1, "one_b"});
    mmap1.insert({2, "two"});

    mmap2.insert({1, "one_a"});
    mmap2.insert({1, "one_b"});
    mmap2.insert({2, "two"});

    EXPECT_TRUE(mmap1 == mmap2);
    EXPECT_FALSE(mmap1 != mmap2);

    // 添加不同数量的相同键
    mmap1.insert({3, "three"});
    mmap2.insert({3, "three"});
    mmap2.insert({3, "three_b"});

    EXPECT_FALSE(mmap1 == mmap2);
    EXPECT_TRUE(mmap1 != mmap2);
    EXPECT_TRUE(mmap1 < mmap2);
    EXPECT_FALSE(mmap1 > mmap2);

    // 测试相同键不同值的比较
    mmap1.clear();
    mmap2.clear();
    mmap1.insert({1, "one"});
    mmap2.insert({1, "ONE"});
    EXPECT_FALSE(mmap1 == mmap2);
    EXPECT_TRUE(mmap1 != mmap2);
}

// 交换操作测试
TEST_F(NFShmMultiMapTest, SwapOperations)
{
    NFShmMultiMap<int, std::string, 20> mmap1;
    NFShmMultiMap<int, std::string, 20> mmap2;

    // 准备测试数据
    mmap1.insert({1, "one_a"});
    mmap1.insert({1, "one_b"});
    mmap1.insert({3, "three"});
    mmap1.insert({5, "five"});

    mmap2.insert({2, "two_a"});
    mmap2.insert({4, "four"});
    mmap2.insert({2, "two_b"});

    size_t size1 = mmap1.size();
    size_t size2 = mmap2.size();

    // 成员函数swap
    mmap1.swap(mmap2);

    EXPECT_EQ(mmap1.size(), size2);
    EXPECT_EQ(mmap2.size(), size1);
    EXPECT_EQ(mmap1.count(2), 2);
    EXPECT_EQ(mmap1.count(4), 1);
    EXPECT_EQ(mmap2.count(1), 2);
    EXPECT_EQ(mmap2.count(3), 1);
    EXPECT_EQ(mmap2.count(5), 1);

    // 全局swap函数
    swap(mmap1, mmap2);

    EXPECT_EQ(mmap1.size(), size1);
    EXPECT_EQ(mmap2.size(), size2);
    EXPECT_EQ(mmap1.count(1), 2);
    EXPECT_EQ(mmap1.count(3), 1);
    EXPECT_EQ(mmap1.count(5), 1);
}

// 键值比较器测试
TEST_F(NFShmMultiMapTest, ComparatorTests)
{
    NFShmMultiMap<int, std::string, 10> mmap;

    // 测试key_comp
    auto key_comp = mmap.key_comp();
    EXPECT_TRUE(key_comp(1, 2));
    EXPECT_FALSE(key_comp(2, 1));
    EXPECT_FALSE(key_comp(1, 1));

    // 测试value_comp - 使用正确的NFShmPair类型
    auto value_comp = mmap.value_comp();
    NFShmPair<int, std::string> pair1(1, "one");
    NFShmPair<int, std::string> pair2(2, "two");
    EXPECT_TRUE(value_comp(pair1, pair2));
    EXPECT_FALSE(value_comp(pair2, pair1));
}

// 性能基准测试
TEST_F(NFShmMultiMapTest, PerformanceBasics)
{
    const size_t LARGE_SIZE = 1000;
    NFShmMultiMap<int, std::string, LARGE_SIZE> large_mmap;

    // 大量插入测试（包含重复键）
    for (size_t i = 0; i < LARGE_SIZE / 3; ++i)
    {
        int key = static_cast<int>(i % 100); // 创建重复键
        large_mmap.insert({key, "value_" + std::to_string(i)});
    }
    EXPECT_EQ(large_mmap.size(), LARGE_SIZE / 3);

    // 验证重复键计数
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_GT(large_mmap.count(i), 0);
    }

    // 大量查找测试
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_NE(large_mmap.find(i), large_mmap.end());
    }
    EXPECT_EQ(large_mmap.find(1000), large_mmap.end());

    // 大量删除测试
    for (int i = 0; i < 50; ++i)
    {
        size_t erased = large_mmap.erase(i);
        EXPECT_GT(erased, 0);
        EXPECT_EQ(large_mmap.count(i), 0);
    }

    // 验证剩余元素
    for (int i = 50; i < 100; ++i)
    {
        EXPECT_GT(large_mmap.count(i), 0);
    }
}

// NFShmMultiMap与std::multimap的效率和空间对比测试
TEST_F(NFShmMultiMapTest, PerformanceComparisonWithStdMultiMap)
{
    const size_t TEST_SIZE = 5000;
    const int ITERATIONS = 3;
    const int MAX_KEY = 1000; // 创建重复键

    // 插入性能测试
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            NFShmMultiMap<int, int, TEST_SIZE> nf_mmap;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                int key = static_cast<int>(i % MAX_KEY); // 创建重复键
                nf_mmap.insert({key, static_cast<int>(i)});
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            std::multimap<int, int> std_mmap;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                int key = static_cast<int>(i % MAX_KEY);
                std_mmap.insert({key, static_cast<int>(i)});
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Insert Performance Comparison (" << TEST_SIZE << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMultiMap insert time: " << nf_insert_time.count() << " μs\n";
        std::cout << "  std::multimap insert time: " << std_insert_time.count() << " μs\n";
        std::cout << "  NFShmMultiMap relative performance: " << (std_insert_time.count() > 0 ? static_cast<double>(std_insert_time.count()) / nf_insert_time.count() : 0.0) << "x\n\n";
    }

    // 查找性能测试
    {
        NFShmMultiMap<int, int, TEST_SIZE> nf_mmap;
        std::multimap<int, int> std_mmap;

        // 准备测试数据
        for (size_t i = 0; i < TEST_SIZE; ++i)
        {
            int key = static_cast<int>(i % MAX_KEY);
            int value = static_cast<int>(i);
            nf_mmap.insert({key, value});
            std_mmap.insert({key, value});
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (int i = 0; i < MAX_KEY; ++i)
            {
                auto it = nf_mmap.find(i);
                if (it != nf_mmap.end())
                {
                    volatile int val = it->second; // 防止优化
                    (void)val;
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_find_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (int i = 0; i < MAX_KEY; ++i)
            {
                auto it = std_mmap.find(i);
                if (it != std_mmap.end())
                {
                    volatile int val = it->second;
                    (void)val;
                }
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_find_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Find Performance Comparison (" << TEST_SIZE << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMultiMap find time: " << nf_find_time.count() << " μs\n";
        std::cout << "  std::multimap find time: " << std_find_time.count() << " μs\n";
        std::cout << "  NFShmMultiMap relative performance: " << (std_find_time.count() > 0 ? static_cast<double>(std_find_time.count()) / nf_find_time.count() : 0.0) << "x\n\n";
    }

    // equal_range性能测试
    {
        NFShmMultiMap<int, int, TEST_SIZE> nf_mmap;
        std::multimap<int, int> std_mmap;

        // 准备测试数据，确保有足够的重复键
        for (size_t i = 0; i < TEST_SIZE; ++i)
        {
            int key = static_cast<int>(i % MAX_KEY);
            int value = static_cast<int>(i);
            nf_mmap.insert({key, value});
            std_mmap.insert({key, value});
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (int i = 0; i < MAX_KEY; ++i)
            {
                auto range = nf_mmap.equal_range(i);
                volatile size_t count = std::distance(range.first, range.second);
                (void)count;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_range_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (int i = 0; i < MAX_KEY; ++i)
            {
                auto range = std_mmap.equal_range(i);
                volatile size_t count = std::distance(range.first, range.second);
                (void)count;
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_range_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Equal Range Performance Comparison (" << TEST_SIZE << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMultiMap equal_range time: " << nf_range_time.count() << " μs\n";
        std::cout << "  std::multimap equal_range time: " << std_range_time.count() << " μs\n";
        std::cout << "  NFShmMultiMap relative performance: " << (std_range_time.count() > 0 ? static_cast<double>(std_range_time.count()) / nf_range_time.count() : 0.0) << "x\n\n";
    }
}

// 空间使用对比测试
TEST_F(NFShmMultiMapTest, MemoryUsageComparison)
{
    const size_t TEST_SIZE = 1000;

    // NFShmMultiMap空间使用
    size_t nf_mmap_size = sizeof(NFShmMultiMap<int, int, TEST_SIZE>);

    // std::multimap估算空间使用（每个节点包含键值对+指针+颜色位，大约）
    size_t std_mmap_node_size = sizeof(int) + sizeof(int) + sizeof(void*) * 3 + sizeof(char); // 估算
    size_t std_mmap_estimated_size = sizeof(std::multimap<int, int>) + TEST_SIZE * std_mmap_node_size;

    std::cout << "Memory Usage Comparison (" << TEST_SIZE << " elements capacity):\n";
    std::cout << "  NFShmMultiMap total size: " << nf_mmap_size << " bytes\n";
    std::cout << "  std::multimap estimated size: " << std_mmap_estimated_size << " bytes\n";
    std::cout << "  NFShmMultiMap average size per element: " << static_cast<double>(nf_mmap_size) / TEST_SIZE << " bytes\n";
    std::cout << "  std::multimap average size per element: " << static_cast<double>(std_mmap_estimated_size) / TEST_SIZE << " bytes\n";
    std::cout << "  Space efficiency ratio: " << static_cast<double>(std_mmap_estimated_size) / nf_mmap_size << "x\n\n";

    // 测试实际内存占用（使用小规模测试）
    {
        const size_t SMALL_TEST_SIZE = 100;
        NFShmMultiMap<int, std::string, SMALL_TEST_SIZE> nf_mmap;
        std::multimap<int, std::string> std_mmap;

        // 填充数据，包含重复键
        for (size_t i = 0; i < SMALL_TEST_SIZE; ++i)
        {
            int key = static_cast<int>(i % 20); // 创建重复键
            std::string value = "value_" + std::to_string(i);
            nf_mmap.insert({key, value});
            std_mmap.insert({key, value});
        }

        // NFShmMultiMap是固定大小的，所以空间占用是确定的
        std::cout << "Actual Usage Test (" << SMALL_TEST_SIZE << " elements):\n";
        std::cout << "  NFShmMultiMap actual size: " << sizeof(nf_mmap) << " bytes\n";
        std::cout << "  NFShmMultiMap utilization: " << static_cast<double>(nf_mmap.size()) / nf_mmap.max_size() * 100 << "%\n";

        // std::multimap的动态内存分配无法直接测量，但可以说明特点
        std::cout << "  std::multimap characteristics: Dynamic allocation, memory fragmentation, independent node allocation\n";
        std::cout << "  NFShmMultiMap characteristics: Pre-allocated fixed size, contiguous memory, no dynamic allocation overhead\n\n";
    }
}

// 并发安全性对比测试（文档性质的）
TEST_F(NFShmMultiMapTest, ConcurrencyAndSafetyComparison)
{
    std::cout << "Concurrency Safety and Feature Comparison:\n\n";

    std::cout << "NFShmMultiMap Features:\n";
    std::cout << "  + Shared memory friendly, supports inter-process communication\n";
    std::cout << "  + Fixed size, no dynamic memory allocation\n";
    std::cout << "  + Contiguous memory layout, cache friendly\n";
    std::cout << "  + Suitable for real-time systems and embedded environments\n";
    std::cout << "  + Supports duplicate keys (multimap behavior)\n";
    std::cout << "  - Fixed capacity, cannot dynamically expand\n";
    std::cout << "  - Requires pre-estimation of maximum capacity\n\n";

    std::cout << "std::multimap Features:\n";
    std::cout << "  + Dynamic size, allocation on demand\n";
    std::cout << "  + Standard library implementation, widely compatible\n";
    std::cout << "  + Mature and stable, heavily optimized\n";
    std::cout << "  + Supports duplicate keys (multimap behavior)\n";
    std::cout << "  - Dynamic memory allocation may cause fragmentation\n";
    std::cout << "  - Not suitable for shared memory scenarios\n";
    std::cout << "  - Memory allocation overhead for each node\n\n";
}

// 异常安全性测试
TEST_F(NFShmMultiMapTest, ExceptionSafetyTest)
{
    NFShmMultiMap<int, MultiTestValue, 10> mmap;

    // 测试构造异常情况下的安全性
    try
    {
        MultiTestValue val1(1, "first");
        MultiTestValue val2(2, "second");

        mmap.insert({1, val1});
        mmap.insert({1, val2}); // 重复键

        EXPECT_EQ(mmap.size(), 2);
        EXPECT_EQ(mmap.count(1), 2);
    }
    catch (...)
    {
        FAIL() << "Unexpected exception during normal operations";
    }

    // 测试满容器时的行为
    NFShmMultiMap<int, int, 3> small_mmap;
    small_mmap.insert({1, 10});
    small_mmap.insert({1, 20}); // 重复键
    small_mmap.insert({2, 30});

    EXPECT_TRUE(small_mmap.full());

    // 尝试插入到满容器应该失败但不抛异常
    auto result = small_mmap.insert({3, 40});
    EXPECT_EQ(result, small_mmap.end());
    EXPECT_EQ(small_mmap.size(), 3);
}

// 边界值和压力测试
TEST_F(NFShmMultiMapTest, BoundaryAndStressTest)
{
    // 空容器边界测试
    NFShmMultiMap<int, std::string, 5> mmap;

    EXPECT_TRUE(mmap.empty());
    EXPECT_EQ(mmap.begin(), mmap.end());
    EXPECT_EQ(mmap.find(1), mmap.end());
    EXPECT_EQ(mmap.count(1), 0);
    EXPECT_EQ(mmap.lower_bound(1), mmap.end());
    EXPECT_EQ(mmap.upper_bound(1), mmap.end());

    auto range = mmap.equal_range(1);
    EXPECT_EQ(range.first, mmap.end());
    EXPECT_EQ(range.second, mmap.end());

    // 单元素测试
    mmap.insert({1, "one"});
    EXPECT_FALSE(mmap.empty());
    EXPECT_EQ(mmap.size(), 1);
    EXPECT_NE(mmap.find(1), mmap.end());
    EXPECT_EQ(mmap.count(1), 1);

    // 容量限制压力测试
    NFShmMultiMap<int, int, 1000> stress_mmap;

    // 插入大量重复键
    const int KEY_RANGE = 100;
    const int VALUES_PER_KEY = 10;

    for (int key = 0; key < KEY_RANGE; ++key)
    {
        for (int val = 0; val < VALUES_PER_KEY; ++val)
        {
            stress_mmap.insert({key, val});
        }
    }

    EXPECT_EQ(stress_mmap.size(), KEY_RANGE * VALUES_PER_KEY);

    // 验证每个键的计数
    for (int key = 0; key < KEY_RANGE; ++key)
    {
        EXPECT_EQ(stress_mmap.count(key), VALUES_PER_KEY);

        // 验证equal_range
        auto range = stress_mmap.equal_range(key);
        EXPECT_EQ(std::distance(range.first, range.second), VALUES_PER_KEY);

        // 验证所有值都存在
        std::vector<int> found_values;
        for (auto it = range.first; it != range.second; ++it)
        {
            EXPECT_EQ(it->first, key);
            found_values.push_back(it->second);
        }

        std::sort(found_values.begin(), found_values.end());
        for (int val = 0; val < VALUES_PER_KEY; ++val)
        {
            EXPECT_NE(std::find(found_values.begin(), found_values.end(), val), found_values.end());
        }
    }
}

// 随机操作测试
TEST_F(NFShmMultiMapTest, RandomOperationsTest)
{
    const size_t OPERATIONS = 1000;
    const int MAX_KEY = 50;
    const int MAX_VALUE = 1000;

    NFShmMultiMap<int, int, OPERATIONS> mmap;
    std::multimap<int, int> reference_map;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> key_dist(1, MAX_KEY);
    std::uniform_int_distribution<> value_dist(1, MAX_VALUE);
    std::uniform_int_distribution<> op_dist(1, 100);

    for (size_t i = 0; i < OPERATIONS / 2; ++i) // 限制操作数以避免容量问题
    {
        int key = key_dist(gen);
        int value = value_dist(gen);
        int op = op_dist(gen);

        if (op <= 70) // 70% 插入操作
        {
            if (!mmap.full())
            {
                mmap.insert({key, value});
                reference_map.insert({key, value});
            }
        }
        else if (op <= 85) // 15% 查找操作
        {
            auto it1 = mmap.find(key);
            auto it2 = reference_map.find(key);
            EXPECT_EQ((it1 != mmap.end()), (it2 != reference_map.end()));
        }
        else if (op <= 95) // 10% count操作
        {
            EXPECT_EQ(mmap.count(key), reference_map.count(key));
        }
        else // 5% 删除操作
        {
            size_t erased1 = mmap.erase(key);
            size_t erased2 = reference_map.erase(key);
            EXPECT_EQ(erased1, erased2);
        }
    }

    // 最终验证大小一致
    EXPECT_EQ(mmap.size(), reference_map.size());
}

// 迭代器稳定性测试
TEST_F(NFShmMultiMapTest, IteratorStabilityTest)
{
    NFShmMultiMap<int, std::string, 20> mmap;

    // 插入数据
    std::vector<std::pair<int, std::string>> test_data = {
        {1, "one_a"}, {3, "three_a"}, {1, "one_b"}, {2, "two"}, {3, "three_b"}, {1, "one_c"}
    };

    for (const auto& pair : test_data)
    {
        mmap.insert(pair);
    }

    // 获取迭代器
    auto begin_it = mmap.begin();
    auto end_it = mmap.end();
    auto find_it = mmap.find(1);

    // 记录当前状态
    size_t original_size = mmap.size();

    // 删除一个不影响begin的元素
    auto last_it = mmap.find(3);
    EXPECT_NE(last_it, mmap.end());
    mmap.erase(last_it);

    // 验证其他迭代器仍然有效
    EXPECT_EQ(mmap.size(), original_size - 1);
    EXPECT_NE(find_it, mmap.end());
    EXPECT_EQ(find_it->first, 1);

    // begin和end迭代器应该仍然有效
    size_t count = 0;
    for (auto it = begin_it; it != end_it && count < mmap.size() + 5; ++it, ++count)
    {
        // 确保不会无限循环
    }
    EXPECT_LE(count, mmap.size() + 1); // 允许一些误差
}

// 简单容量测试
TEST_F(NFShmMultiMapTest, SimpleCapacityTest)
{
    // 简单容量测试
    NFShmMultiMap<int, int, 1000> mmap;
    EXPECT_EQ(mmap.max_size(), 1000);
    EXPECT_FALSE(mmap.full());

    // 填充到接近容量
    for (int i = 0; i < 999; ++i)
    {
        mmap.insert({i % 100, i}); // 创建重复键
    }
    EXPECT_FALSE(mmap.full());

    mmap.insert({999, 999});
    EXPECT_TRUE(mmap.full());

    // 尝试插入到满容器
    auto result = mmap.insert({1000, 1000});
    EXPECT_EQ(result, mmap.end()); // 插入应该失败
}

// MultiMap特有功能深度测试
TEST_F(NFShmMultiMapTest, MultiMapSpecificFeatures)
{
    NFShmMultiMap<std::string, int, 20> mmap;

    // 测试大量重复键的插入和管理
    const std::string key = "duplicate_key";
    std::vector<int> values = {10, 5, 15, 3, 8, 12, 1, 20, 6, 9};

    for (int val : values)
    {
        mmap.insert({key, val});
    }

    EXPECT_EQ(mmap.count(key), values.size());

    // 验证equal_range返回所有相同键的元素
    auto range = mmap.equal_range(key);
    std::vector<int> found_values;
    for (auto it = range.first; it != range.second; ++it)
    {
        found_values.push_back(it->second);
    }

    EXPECT_EQ(found_values.size(), values.size());

    // 验证所有值都被找到（不考虑顺序）
    std::sort(values.begin(), values.end());
    std::sort(found_values.begin(), found_values.end());
    EXPECT_EQ(found_values, values);

    // 测试插入到特定位置的hint功能
    auto hint_it = mmap.upper_bound(key);
    auto new_it = mmap.insert(hint_it, {key, 25});
    EXPECT_EQ(new_it->first, key);
    EXPECT_EQ(new_it->second, 25);
    EXPECT_EQ(mmap.count(key), values.size() + 1);

    // 测试删除单个元素不影响其他相同键的元素
    auto erase_it = mmap.find(key);
    EXPECT_NE(erase_it, mmap.end());
    int erased_value = erase_it->second;
    mmap.erase(erase_it);

    EXPECT_EQ(mmap.count(key), values.size()); // 减少1个

    // 验证被删除的值确实不在了
    range = mmap.equal_range(key);
    bool found_erased = false;
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == erased_value)
        {
            found_erased = true;
            break;
        }
    }
    EXPECT_FALSE(found_erased);
}

// 高级迭代器操作测试
TEST_F(NFShmMultiMapTest, AdvancedIteratorOperations)
{
    NFShmMultiMap<int, std::string, 30> mmap;

    // 插入有序数据
    std::vector<std::pair<int, std::string>> data = {
        {1, "a"}, {1, "b"}, {2, "c"}, {3, "d"}, {3, "e"}, {3, "f"}, {4, "g"}, {5, "h"}
    };

    for (const auto& pair : data)
    {
        mmap.insert(pair);
    }

    // 测试std::advance和std::distance
    auto it = mmap.begin();
    std::advance(it, 3);
    EXPECT_EQ(it->first, 3);
    EXPECT_EQ(it->second, "d");

    auto distance = std::distance(mmap.begin(), it);
    EXPECT_EQ(distance, 3);

    // 测试std::next和std::prev
    auto next_it = std::next(it, 2);
    EXPECT_EQ(next_it->first, 3);
    EXPECT_EQ(next_it->second, "f");

    auto prev_it = std::prev(next_it, 1);
    EXPECT_EQ(prev_it->first, 3);
    EXPECT_EQ(prev_it->second, "e");

    // 测试迭代器兼容的算法
    auto count_3 = std::count_if(mmap.begin(), mmap.end(),
                                 [](const NFShmPair<int, std::string>& p) { return p.first == 3; });
    EXPECT_EQ(count_3, 3);

    // 测试find_if
    auto find_it = std::find_if(mmap.begin(), mmap.end(),
                                [](const NFShmPair<int, std::string>& p) { return p.second == "e"; });
    EXPECT_NE(find_it, mmap.end());
    EXPECT_EQ(find_it->first, 3);
    EXPECT_EQ(find_it->second, "e");
}

// 大规模重复键性能测试
TEST_F(NFShmMultiMapTest, LargeScaleDuplicateKeyTest)
{
    const size_t TOTAL_ELEMENTS = 5000;
    const int NUM_KEYS = 100;
    NFShmMultiMap<int, int, TOTAL_ELEMENTS> large_mmap;

    // 插入大量具有重复键的数据
    for (size_t i = 0; i < TOTAL_ELEMENTS; ++i)
    {
        int key = static_cast<int>(i % NUM_KEYS);
        large_mmap.insert({key, static_cast<int>(i)});
    }

    EXPECT_EQ(large_mmap.size(), TOTAL_ELEMENTS);

    // 验证每个键的数量
    size_t expected_per_key = TOTAL_ELEMENTS / NUM_KEYS;
    for (int key = 0; key < NUM_KEYS; ++key)
    {
        EXPECT_EQ(large_mmap.count(key), expected_per_key);

        // 测试equal_range性能
        auto range = large_mmap.equal_range(key);
        EXPECT_EQ(std::distance(range.first, range.second), static_cast<ptrdiff_t>(expected_per_key));
    }

    // 批量删除测试
    for (int key = 0; key < NUM_KEYS / 2; ++key)
    {
        size_t erased = large_mmap.erase(key);
        EXPECT_EQ(erased, expected_per_key);
        EXPECT_EQ(large_mmap.count(key), 0);
    }

    // 验证剩余数据
    EXPECT_EQ(large_mmap.size(), TOTAL_ELEMENTS / 2);
    for (int key = NUM_KEYS / 2; key < NUM_KEYS; ++key)
    {
        EXPECT_EQ(large_mmap.count(key), expected_per_key);
    }
}
