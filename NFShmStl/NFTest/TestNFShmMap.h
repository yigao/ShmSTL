// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmMap.h
//    @Author           :    Assistant
//    @Date             :    2025/1/16
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmMap
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmMap.h"
#include "NFComm/NFShmStl/NFShmPair.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>

// 自定义值类型用于测试
class TestValue
{
public:
    TestValue() : id(0), name("default")
    {
        ++constructor_count;
    }

    TestValue(int i) : id(i), name("value_" + std::to_string(i))
    {
        ++constructor_count;
    }

    TestValue(int i, const std::string& n) : id(i), name(n)
    {
        ++constructor_count;
    }

    TestValue(const TestValue& other) : id(other.id), name(other.name)
    {
        ++constructor_count;
    }

    TestValue& operator=(const TestValue& other)
    {
        if (this != &other)
        {
            id = other.id;
            name = other.name;
        }
        return *this;
    }

    ~TestValue()
    {
        ++destructor_count;
    }

    bool operator==(const TestValue& other) const
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

int TestValue::constructor_count = 0;
int TestValue::destructor_count = 0;

class NFShmMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestValue::reset_counters();
    }

    void TearDown() override
    {
        // 验证没有内存泄漏
        EXPECT_EQ(TestValue::constructor_count, TestValue::destructor_count);
    }
};

// 基本功能测试
TEST_F(NFShmMapTest, BasicOperationsWithIntString)
{
    NFShmMap<int, std::string, 10> map;

    // 测试空容器
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.max_size(), 10);
    EXPECT_FALSE(map.full());

    // 测试插入
    auto result = map.insert({1, "one"});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, 1);
    EXPECT_EQ(result.first->second, "one");
    EXPECT_EQ(map.size(), 1);
    EXPECT_FALSE(map.empty());

    // 测试重复键插入
    result = map.insert({1, "another one"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second, "one"); // 保持原值
    EXPECT_EQ(map.size(), 1);

    // 测试operator[]插入
    map[2] = "two";
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[2], "two");

    // 测试operator[]访问
    map[3] = "three";
    EXPECT_EQ(map[3], "three");
    map[3] = "modified three";
    EXPECT_EQ(map[3], "modified three");

    // 测试查找
    auto it = map.find(1);
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");

    it = map.find(10);
    EXPECT_EQ(it, map.end());

    // 测试count
    EXPECT_EQ(map.count(1), 1);
    EXPECT_EQ(map.count(10), 0);

    // 测试at方法
    EXPECT_EQ(map.at(2), "two");
    EXPECT_EQ(&map.at(100), map.GetStaticError());

    // 测试lower_bound和upper_bound
    auto lower = map.lower_bound(2);
    auto upper = map.upper_bound(2);
    EXPECT_NE(lower, map.end());
    EXPECT_EQ(lower->first, 2);
    EXPECT_NE(upper, lower);

    // 测试equal_range
    auto range = map.equal_range(2);
    EXPECT_EQ(range.first, lower);
    EXPECT_EQ(range.second, upper);
    EXPECT_EQ(std::distance(range.first, range.second), 1);

    // 测试删除
    size_t erased = map.erase(1);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find(1), map.end());

    // 测试迭代器删除
    it = map.find(2);
    EXPECT_NE(it, map.end());
    map.erase(it);
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map.find(2), map.end());
}

// 自定义类型测试
TEST_F(NFShmMapTest, CustomTypeOperations)
{
    NFShmMap<std::string, TestValue, 8> map;

    // 测试自定义类型插入
    TestValue val1(1, "first");
    TestValue val2(2, "second");
    TestValue val3(3, "third");

    auto result = map.insert({"key1", val1});
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, "key1");
    EXPECT_EQ(result.first->second.id, 1);
    EXPECT_EQ(result.first->second.name, "first");

    map["key2"] = val2;
    map["key3"] = val3;
    EXPECT_EQ(map.size(), 3);

    // 测试自定义类型访问
    EXPECT_EQ(map["key1"].id, 1);
    EXPECT_EQ(map["key1"].name, "first");
    EXPECT_EQ(map.at("key2").id, 2);

    // 测试自定义类型修改
    map["key1"] = TestValue(10, "modified");
    EXPECT_EQ(map["key1"].id, 10);
    EXPECT_EQ(map["key1"].name, "modified");

    // 测试键排序（字符串按字典序）
    std::vector<std::string> expected_keys = {"key1", "key2", "key3"};
    std::vector<std::string> actual_keys;
    for (const auto& pair : map)
    {
        actual_keys.push_back(pair.first);
    }
    EXPECT_EQ(actual_keys, expected_keys);

    // 测试查找和删除
    auto it = map.find("key2");
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->second.id, 2);

    map.erase("key2");
    EXPECT_EQ(map.find("key2"), map.end());
    EXPECT_EQ(map.size(), 2);
}

// 构造函数测试
TEST_F(NFShmMapTest, ConstructorTests)
{
    // 默认构造函数
    NFShmMap<int, std::string, 5> map1;
    EXPECT_TRUE(map1.empty());

    // 范围构造函数（迭代器）
    std::vector<std::pair<int, std::string>> vec = {
        {1, "one"}, {3, "three"}, {5, "five"}
    };
    NFShmMap<int, std::string, 10> map2(vec.begin(), vec.end());
    EXPECT_EQ(map2.size(), 3);
    EXPECT_EQ(map2[1], "one");
    EXPECT_EQ(map2[3], "three");
    EXPECT_EQ(map2[5], "five");

    // 拷贝构造函数
    NFShmMap<int, std::string, 10> map3(map2);
    EXPECT_EQ(map3.size(), map2.size());
    EXPECT_TRUE(std::equal(map2.begin(), map2.end(), map3.begin()));

    // 指针范围构造函数
    using PairType = NFShmPair<const int, std::string>;
    PairType arr[] = {{2, "two"}, {4, "four"}, {6, "six"}};
    NFShmMap<int, std::string, 10> map4(arr, arr + 3);
    EXPECT_EQ(map4.size(), 3);
    EXPECT_EQ(map4[2], "two");
    EXPECT_EQ(map4[4], "four");
    EXPECT_EQ(map4[6], "six");

    // const_iterator范围构造函数
    NFShmMap<int, std::string, 10> map5(map2.begin(), map2.end());
    EXPECT_EQ(map5.size(), map2.size());
    EXPECT_TRUE(std::equal(map2.begin(), map2.end(), map5.begin()));
}

// STL兼容性测试
TEST_F(NFShmMapTest, STLCompatibility)
{
    // 从std::map构造
    std::map<int, std::string> std_map = {{1, "one"}, {3, "three"}, {5, "five"}};
    NFShmMap<int, std::string, 10> nf_map(std_map);
    EXPECT_EQ(nf_map.size(), std_map.size());
    for (const auto& pair : std_map)
    {
        EXPECT_EQ(nf_map[pair.first], pair.second);
    }

    // 从std::unordered_map构造
    std::unordered_map<int, std::string> unordered_map = {{2, "two"}, {4, "four"}, {6, "six"}};
    NFShmMap<int, std::string, 10> nf_map2(unordered_map);
    EXPECT_EQ(nf_map2.size(), unordered_map.size());
    for (const auto& pair : unordered_map)
    {
        EXPECT_EQ(nf_map2[pair.first], pair.second);
    }

    // 赋值操作符测试
    std::map<int, std::string> another_std_map = {{10, "ten"}, {20, "twenty"}, {30, "thirty"}};
    nf_map = another_std_map;
    EXPECT_EQ(nf_map.size(), another_std_map.size());
    for (const auto& pair : another_std_map)
    {
        EXPECT_EQ(nf_map[pair.first], pair.second);
    }

    // 从std::unordered_map赋值
    std::unordered_map<int, std::string> another_unordered_map = {{40, "forty"}, {50, "fifty"}};
    nf_map = another_unordered_map;
    EXPECT_EQ(nf_map.size(), another_unordered_map.size());
    for (const auto& pair : another_unordered_map)
    {
        EXPECT_EQ(nf_map[pair.first], pair.second);
    }
}

// 迭代器测试
TEST_F(NFShmMapTest, IteratorTests)
{
    NFShmMap<int, std::string, 10> map;
    std::vector<std::pair<int, std::string>> values = {
        {5, "five"}, {1, "one"}, {9, "nine"}, {3, "three"}, {7, "seven"}
    };

    for (const auto& pair : values)
    {
        map.insert(pair);
    }

    // 正向迭代器测试（按键排序）
    std::vector<int> sorted_keys = {1, 3, 5, 7, 9};
    std::vector<int> iterated_keys;
    for (auto it = map.begin(); it != map.end(); ++it)
    {
        iterated_keys.push_back(it->first);
    }
    EXPECT_EQ(iterated_keys, sorted_keys);

    // 反向迭代器测试
    std::vector<int> reverse_sorted = {9, 7, 5, 3, 1};
    std::vector<int> reverse_iterated;
    for (auto it = map.rbegin(); it != map.rend(); ++it)
    {
        reverse_iterated.push_back(it->first);
    }
    EXPECT_EQ(reverse_iterated, reverse_sorted);

    // const迭代器测试
    const NFShmMap<int, std::string, 10>& const_map = map;
    std::vector<int> const_iterated;
    for (auto it = const_map.cbegin(); it != const_map.cend(); ++it)
    {
        const_iterated.push_back(it->first);
    }
    EXPECT_EQ(const_iterated, sorted_keys);

    // 范围for循环测试
    std::vector<int> range_for_keys;
    for (const auto& pair : map)
    {
        range_for_keys.push_back(pair.first);
    }
    EXPECT_EQ(range_for_keys, sorted_keys);

    // 测试迭代器修改值
    for (auto& pair : map)
    {
        pair.second = "modified_" + pair.second;
    }
    EXPECT_EQ(map[1], "modified_one");
    EXPECT_EQ(map[5], "modified_five");
}

// 边界条件测试
TEST_F(NFShmMapTest, BoundaryTests)
{
    NFShmMap<int, std::string, 3> small_map;

    // 测试容量限制
    small_map[1] = "one";
    small_map[2] = "two";
    small_map[3] = "three";
    EXPECT_TRUE(small_map.full());
    EXPECT_EQ(small_map.size(), 3);

    // 尝试插入到满容器
    auto result = small_map.insert({4, "four"});
    EXPECT_FALSE(result.second);
    EXPECT_EQ(small_map.size(), 3);

    // 尝试使用operator[]访问新键（应该失败）
    std::string& ref = small_map[4];
    EXPECT_EQ(small_map.size(), 3); // 不应该插入成功

    // 测试清空
    small_map.clear();
    EXPECT_TRUE(small_map.empty());
    EXPECT_EQ(small_map.size(), 0);
    EXPECT_FALSE(small_map.full());

    // 重新插入
    small_map[10] = "ten";
    EXPECT_EQ(small_map.size(), 1);
    EXPECT_EQ(small_map[10], "ten");
}

// emplace操作测试
TEST_F(NFShmMapTest, EmplaceOperations)
{
    NFShmMap<std::string, TestValue, 8> map;

    // 测试emplace
    auto result = map.emplace("key1", TestValue(1, "first"));
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first->first, "key1");
    EXPECT_EQ(result.first->second.id, 1);
    EXPECT_EQ(result.first->second.name, "first");

    // 测试emplace重复键
    result = map.emplace("key1", TestValue(2, "duplicate"));
    EXPECT_FALSE(result.second);
    EXPECT_EQ(result.first->second.id, 1); // 保持原值
    EXPECT_EQ(result.first->second.name, "first");

    // 测试emplace_hint
    auto it = map.emplace_hint(map.end(), "key2", TestValue(2, "second"));
    EXPECT_EQ(it->first, "key2");
    EXPECT_EQ(it->second.id, 2);
    EXPECT_EQ(it->second.name, "second");

    EXPECT_EQ(map.size(), 2);
}

// 删除操作测试
TEST_F(NFShmMapTest, EraseOperations)
{
    NFShmMap<int, std::string, 10> map;

    // 准备测试数据
    std::vector<std::pair<int, std::string>> values = {
        {1, "one"}, {3, "three"}, {5, "five"}, {7, "seven"}, {9, "nine"}
    };
    for (const auto& pair : values)
    {
        map.insert(pair);
    }
    EXPECT_EQ(map.size(), 5);

    // 测试删除单个迭代器
    auto it = map.find(3);
    EXPECT_NE(it, map.end());
    map.erase(it);
    EXPECT_EQ(map.size(), 4);
    EXPECT_EQ(map.find(3), map.end());

    // 测试按键删除
    size_t erased = map.erase(7);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.find(7), map.end());

    // 测试范围删除
    auto first = map.find(1);
    auto last = map.find(9);
    ++last; // 指向end或下一个元素
    map.erase(first, last);
    EXPECT_EQ(map.size(), 0); // 应该删除所有剩余元素，大小为0
    EXPECT_EQ(map.find(9), map.end()); // 键9应该被删除
    EXPECT_EQ(map.find(1), map.end());
    EXPECT_EQ(map.find(5), map.end());
}

// 批量操作测试
TEST_F(NFShmMapTest, BatchOperations)
{
    NFShmMap<int, std::string, 20> map;

    // 批量插入
    std::vector<std::pair<int, std::string>> values = {
        {1, "one"}, {3, "three"}, {5, "five"}, {7, "seven"},
        {9, "nine"}, {11, "eleven"}, {13, "thirteen"}, {15, "fifteen"}
    };

    for (const auto& pair : values)
    {
        map.insert(pair);
    }
    EXPECT_EQ(map.size(), values.size());

    // 验证所有元素
    for (const auto& pair : values)
    {
        EXPECT_EQ(map[pair.first], pair.second);
    }

    // 批量删除
    std::vector<int> to_erase = {3, 7, 11, 15};
    for (int key : to_erase)
    {
        map.erase(key);
    }
    EXPECT_EQ(map.size(), values.size() - to_erase.size());

    // 验证删除结果
    std::vector<int> remaining = {1, 5, 9, 13};
    for (int key : remaining)
    {
        EXPECT_NE(map.find(key), map.end());
    }
    for (int key : to_erase)
    {
        EXPECT_EQ(map.find(key), map.end());
    }
}

// 比较操作符测试
TEST_F(NFShmMapTest, ComparisonOperators)
{
    NFShmMap<int, std::string, 10> map1;
    NFShmMap<int, std::string, 10> map2;

    // 空容器比较
    EXPECT_TRUE(map1 == map2);
    EXPECT_FALSE(map1 != map2);
    EXPECT_FALSE(map1 < map2);
    EXPECT_TRUE(map1 <= map2);
    EXPECT_FALSE(map1 > map2);
    EXPECT_TRUE(map1 >= map2);

    // 添加相同元素
    map1[1] = "one";
    map1[2] = "two";
    map2[1] = "one";
    map2[2] = "two";
    EXPECT_TRUE(map1 == map2);
    EXPECT_FALSE(map1 != map2);

    // 添加不同元素
    map1[3] = "three";
    map2[4] = "four";
    EXPECT_FALSE(map1 == map2);
    EXPECT_TRUE(map1 != map2);
    EXPECT_TRUE(map1 < map2);
    EXPECT_TRUE(map1 <= map2);
    EXPECT_FALSE(map1 > map2);
    EXPECT_FALSE(map1 >= map2);

    // 测试相同键不同值的比较
    map1.clear();
    map2.clear();
    map1[1] = "one";
    map2[1] = "ONE";
    EXPECT_FALSE(map1 == map2);
    EXPECT_TRUE(map1 != map2);
}

// 交换操作测试
TEST_F(NFShmMapTest, SwapOperations)
{
    NFShmMap<int, std::string, 10> map1;
    NFShmMap<int, std::string, 10> map2;

    // 准备测试数据
    map1[1] = "one";
    map1[3] = "three";
    map1[5] = "five";

    map2[2] = "two";
    map2[4] = "four";

    size_t size1 = map1.size();
    size_t size2 = map2.size();

    // 成员函数swap
    map1.swap(map2);

    EXPECT_EQ(map1.size(), size2);
    EXPECT_EQ(map2.size(), size1);
    EXPECT_EQ(map1[2], "two");
    EXPECT_EQ(map1[4], "four");
    EXPECT_EQ(map2[1], "one");
    EXPECT_EQ(map2[3], "three");
    EXPECT_EQ(map2[5], "five");

    // 全局swap函数
    swap(map1, map2);

    EXPECT_EQ(map1.size(), size1);
    EXPECT_EQ(map2.size(), size2);
    EXPECT_EQ(map1[1], "one");
    EXPECT_EQ(map1[3], "three");
    EXPECT_EQ(map1[5], "five");
}

// 键值比较器测试
TEST_F(NFShmMapTest, ComparatorTests)
{
    NFShmMap<int, std::string, 10> map;

    // 测试key_comp
    auto key_comp = map.key_comp();
    EXPECT_TRUE(key_comp(1, 2));
    EXPECT_FALSE(key_comp(2, 1));
    EXPECT_FALSE(key_comp(1, 1));

    // 测试value_comp
    auto value_comp = map.value_comp();
    using PairType = std::pair<const int, std::string>;
    PairType pair1 = {1, "one"};
    PairType pair2 = {2, "two"};
    EXPECT_TRUE(value_comp(pair1, pair2));
    EXPECT_FALSE(value_comp(pair2, pair1));
}

// 性能基准测试
TEST_F(NFShmMapTest, PerformanceBasics)
{
    const size_t LARGE_SIZE = 1000;
    NFShmMap<int, std::string, LARGE_SIZE> large_map;

    // 大量插入测试
    for (size_t i = 0; i < LARGE_SIZE / 2; ++i)
    {
        large_map[static_cast<int>(i)] = "value_" + std::to_string(i);
    }
    EXPECT_EQ(large_map.size(), LARGE_SIZE / 2);

    // 大量查找测试
    for (size_t i = 0; i < LARGE_SIZE / 2; ++i)
    {
        EXPECT_NE(large_map.find(static_cast<int>(i)), large_map.end());
        EXPECT_EQ(large_map[static_cast<int>(i)], "value_" + std::to_string(i));
    }
    EXPECT_EQ(large_map.find(static_cast<int>(LARGE_SIZE)), large_map.end());

    // 大量删除测试
    for (size_t i = 0; i < LARGE_SIZE / 4; ++i)
    {
        large_map.erase(static_cast<int>(i));
    }
    EXPECT_EQ(large_map.size(), LARGE_SIZE / 2 - LARGE_SIZE / 4);

    // 验证删除结果
    for (size_t i = 0; i < LARGE_SIZE / 4; ++i)
    {
        EXPECT_EQ(large_map.find(static_cast<int>(i)), large_map.end());
    }
    for (size_t i = LARGE_SIZE / 4; i < LARGE_SIZE / 2; ++i)
    {
        EXPECT_NE(large_map.find(static_cast<int>(i)), large_map.end());
    }
}

// NFShmMap与std::map的效率和空间对比测试
TEST_F(NFShmMapTest, InsertPerformanceComparisonWithStdMap)
{
    const size_t TEST_SIZE = 10000;
    const int ITERATIONS = 3;

    // Insert performance test
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            NFShmMap<int, int, TEST_SIZE> nf_map;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                nf_map[static_cast<int>(i)] = static_cast<int>(i * 2);
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            std::map<int, int> std_map;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                std_map[static_cast<int>(i)] = static_cast<int>(i * 2);
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Insert Performance Comparison (" << TEST_SIZE << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMap insert time: " << nf_insert_time.count() << " us\n";
        std::cout << "  std::map insert time: " << std_insert_time.count() << " us\n";
        std::cout << "  NFShmMap relative performance: " << (std_insert_time.count() > 0 ? static_cast<double>(std_insert_time.count()) / nf_insert_time.count() : 0.0) << "x\n\n";
    }
}

// NFShmMap与std::map的效率和空间对比测试
TEST_F(NFShmMapTest, FindPerformanceComparisonWithStdMap)
{
    const size_t TEST_SIZE = 10000;
    const int ITERATIONS = 3;

    // Find performance test
    {
        NFShmMap<int, int, TEST_SIZE> nf_map;
        std::map<int, int> std_map;

        // Prepare data
        for (size_t i = 0; i < TEST_SIZE; ++i)
        {
            int key = static_cast<int>(i);
            int value = static_cast<int>(i * 2);
            nf_map[key] = value;
            std_map[key] = value;
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                auto it = nf_map.find(static_cast<int>(i));
                (void)it; // Avoid compiler optimization
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_find_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                auto it = std_map.find(static_cast<int>(i));
                (void)it; // Avoid compiler optimization
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_find_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Find Performance Comparison (" << TEST_SIZE << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMap find time: " << nf_find_time.count() << " us\n";
        std::cout << "  std::map find time: " << std_find_time.count() << " us\n";
        std::cout << "  NFShmMap relative performance: " << (std_find_time.count() > 0 ? static_cast<double>(std_find_time.count()) / nf_find_time.count() : 0.0) << "x\n\n";
    }
}

// NFShmMap与std::map的效率和空间对比测试
TEST_F(NFShmMapTest, ErasePerformanceComparisonWithStdMap)
{
    const size_t TEST_SIZE = 10000;
    const int ITERATIONS = 3;

    // Erase performance test
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            NFShmMap<int, int, TEST_SIZE> nf_map;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                nf_map[static_cast<int>(i)] = static_cast<int>(i * 2);
            }
            for (size_t i = 0; i < TEST_SIZE / 2; ++i)
            {
                nf_map.erase(static_cast<int>(i));
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto nf_erase_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        start = std::chrono::high_resolution_clock::now();

        for (int iter = 0; iter < ITERATIONS; ++iter)
        {
            std::map<int, int> std_map;
            for (size_t i = 0; i < TEST_SIZE; ++i)
            {
                std_map[static_cast<int>(i)] = static_cast<int>(i * 2);
            }
            for (size_t i = 0; i < TEST_SIZE / 2; ++i)
            {
                std_map.erase(static_cast<int>(i));
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto std_erase_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Erase Performance Comparison (" << TEST_SIZE / 2 << " elements, " << ITERATIONS << " iterations):\n";
        std::cout << "  NFShmMap erase time: " << nf_erase_time.count() << " us\n";
        std::cout << "  std::map erase time: " << std_erase_time.count() << " us\n";
        std::cout << "  NFShmMap relative performance: " << (std_erase_time.count() > 0 ? static_cast<double>(std_erase_time.count()) / nf_erase_time.count() : 0.0) << "x\n\n";
    }
}

// Space usage comparison test
TEST_F(NFShmMapTest, MemoryUsageComparison)
{
    const size_t TEST_SIZE = 1000;

    // NFShmMap space usage
    size_t nf_map_size = sizeof(NFShmMap<int, int, TEST_SIZE>);
    
    // std::map estimated space usage (each node contains key-value pair + pointers + color bit approximately)
    size_t std_map_node_size = sizeof(int) + sizeof(int) + sizeof(void*) * 3 + sizeof(char); // Estimate
    size_t std_map_estimated_size = sizeof(std::map<int, int>) + TEST_SIZE * std_map_node_size;

    std::cout << "Memory Usage Comparison (" << TEST_SIZE << " elements capacity):\n";
    std::cout << "  NFShmMap total size: " << nf_map_size << " bytes\n";
    std::cout << "  std::map estimated size: " << std_map_estimated_size << " bytes\n";
    std::cout << "  NFShmMap average size per element: " << static_cast<double>(nf_map_size) / TEST_SIZE << " bytes\n";
    std::cout << "  std::map average size per element: " << static_cast<double>(std_map_estimated_size) / TEST_SIZE << " bytes\n";
    std::cout << "  Space efficiency ratio: " << static_cast<double>(std_map_estimated_size) / nf_map_size << "x\n\n";

    // 测试实际内存占用（使用小规模测试）
    {
        const size_t SMALL_TEST_SIZE = 100;
        NFShmMap<int, std::string, SMALL_TEST_SIZE> nf_map;
        std::map<int, std::string> std_map;

        // Fill data
        for (size_t i = 0; i < SMALL_TEST_SIZE; ++i)
        {
            std::string value = "value_" + std::to_string(i);
            nf_map[static_cast<int>(i)] = value;
            std_map[static_cast<int>(i)] = value;
        }

        // NFShmMap is fixed size, so memory usage is deterministic
        std::cout << "Actual Usage Test (" << SMALL_TEST_SIZE << " elements):\n";
        std::cout << "  NFShmMap actual size: " << sizeof(nf_map) << " bytes\n";
        std::cout << "  NFShmMap utilization: " << static_cast<double>(nf_map.size()) / nf_map.max_size() * 100 << "%\n";
        
        // std::map's dynamic memory allocation cannot be directly measured, but we can explain characteristics
        std::cout << "  std::map characteristics: Dynamic allocation, memory fragmentation, independent node allocation\n";
        std::cout << "  NFShmMap characteristics: Pre-allocated fixed size, contiguous memory, no dynamic allocation overhead\n\n";
    }
}

// Concurrency safety comparison test (documentation)
TEST_F(NFShmMapTest, ConcurrencyAndSafetyComparison)
{
    std::cout << "Concurrency Safety and Feature Comparison:\n\n";
    
    std::cout << "NFShmMap Features:\n";
    std::cout << "  + Shared memory friendly, supports inter-process communication\n";
    std::cout << "  + Fixed size, no dynamic memory allocation\n";
    std::cout << "  + Contiguous memory layout, cache friendly\n";
    std::cout << "  + Suitable for real-time systems and embedded environments\n";
    std::cout << "  - Fixed capacity, cannot dynamically expand\n";
    std::cout << "  - Requires pre-estimation of maximum capacity\n\n";
    
    std::cout << "std::map Features:\n";
    std::cout << "  + Dynamic size, allocation on demand\n";
    std::cout << "  + Standard library implementation, widely compatible\n";
    std::cout << "  + Mature and stable, heavily optimized\n";
    std::cout << "  - Dynamic memory allocation overhead\n";
    std::cout << "  - Memory fragmentation\n";
    std::cout << "  - No shared memory support\n\n";
    
    std::cout << "Use Cases:\n";
    std::cout << "  NFShmMap: Game servers, real-time systems, shared memory applications, memory-sensitive applications\n";
    std::cout << "  std::map: General applications, scenarios with uncertain capacity, standard C++ environments\n\n";

    // Simple capacity test
    NFShmMap<int, int, 1000> nf_map;
    EXPECT_EQ(nf_map.max_size(), 1000);
    EXPECT_FALSE(nf_map.full());
    
    // Fill to near capacity
    for (int i = 0; i < 999; ++i)
    {
        nf_map[i] = i;
    }
    EXPECT_FALSE(nf_map.full());
    
    nf_map[999] = 999;
    EXPECT_TRUE(nf_map.full());
    
    // Try to insert into full container
    auto result = nf_map.insert({1000, 1000});
    EXPECT_FALSE(result.second); // Insert should fail
}
