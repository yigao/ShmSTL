// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashTable.h
//    @Author           :    gaoyi
//    @Date             :    2025/1/27
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashTable
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashTable.h"
#include <string>
#include <functional>

// 测试用的简单键值对结构
struct TestPair
{
    int key;
    std::string value;

    TestPair() : key(0), value("")
    {
    }

    TestPair(int k, const std::string& v) : key(k), value(v)
    {
    }

    bool operator==(const TestPair& other) const
    {
        return key == other.key && value == other.value;
    }
};

// 提取键的函数对象
struct ExtractKey
{
    int operator()(const TestPair& pair) const
    {
        return pair.key;
    }
};

// 哈希函数对象
struct HashFunc
{
    size_t operator()(int key) const
    {
        return std::hash<int>{}(key);
    }
};

// 键比较函数对象
struct EqualKey
{
    bool operator()(int key1, int key2) const
    {
        return key1 == key2;
    }
};

// 定义测试用的哈希表类型
using TestHashTable = NFShmHashTable<TestPair, int, 100, HashFunc, ExtractKey, EqualKey>;

// 基础功能测试
TEST(NFShmHashTableTest, BasicConstruction)
{
    TestHashTable ht;

    EXPECT_EQ(ht.size(), 0);
    EXPECT_EQ(ht.max_size(), 100);
    EXPECT_TRUE(ht.empty());
    EXPECT_FALSE(ht.full());
    EXPECT_EQ(ht.left_size(), 100);
    EXPECT_EQ(ht.bucket_count(), 100);
    EXPECT_EQ(ht.max_bucket_count(), 100);
}

// 插入和查找测试
TEST(NFShmHashTableTest, InsertAndFind)
{
    TestHashTable ht;

    // 测试插入unique
    TestPair pair1(1, "value1");
    auto result1 = ht.insert_unique(pair1);
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->key, 1);
    EXPECT_EQ(result1.first->value, "value1");
    EXPECT_EQ(ht.size(), 1);

    // 测试重复插入unique
    TestPair pair1_dup(1, "value1_dup");
    auto result2 = ht.insert_unique(pair1_dup);
    EXPECT_FALSE(result2.second);
    EXPECT_EQ(ht.size(), 1);

    // 测试查找
    auto find_iter = ht.find(1);
    EXPECT_NE(find_iter, ht.end());
    EXPECT_EQ(find_iter->key, 1);
    EXPECT_EQ(find_iter->value, "value1");

    // 测试查找不存在的键
    auto find_iter2 = ht.find(999);
    EXPECT_EQ(find_iter2, ht.end());
}

// 插入equal测试
TEST(NFShmHashTableTest, InsertEqual)
{
    TestHashTable ht;

    // 插入相同键的多个值
    TestPair pair1(1, "value1");
    TestPair pair2(1, "value2");
    TestPair pair3(1, "value3");

    auto iter1 = ht.insert_equal(pair1);
    auto iter2 = ht.insert_equal(pair2);
    auto iter3 = ht.insert_equal(pair3);

    EXPECT_EQ(ht.size(), 3);
    EXPECT_EQ(ht.count(1), 3);

    // 验证所有值都被插入
    EXPECT_NE(iter1, ht.end());
    EXPECT_NE(iter2, ht.end());
    EXPECT_NE(iter3, ht.end());
}

// 删除测试
TEST(NFShmHashTableTest, Erase)
{
    TestHashTable ht;

    // 插入测试数据
    for (int i = 1; i <= 10; ++i)
    {
        TestPair pair(i*100, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }
    EXPECT_EQ(ht.size(), 10);

    ht.print_structure();

    // 测试按键删除
    size_t erased = ht.erase(100);
    EXPECT_EQ(erased, 1);
    EXPECT_EQ(ht.size(), 9);
    EXPECT_EQ(ht.find(100), ht.end());
    ht.print_structure();

    // 测试按迭代器删除
    auto iter = ht.find(500);
    EXPECT_NE(iter, ht.end());
    auto next_iter = ht.erase(iter);
    EXPECT_EQ(ht.size(), 8);
    EXPECT_EQ(ht.find(500), ht.end());
    ht.print_structure();
    if (next_iter != ht.end())
    {
        EXPECT_NE(next_iter->key, 500);
        EXPECT_EQ(next_iter->key, 300);
    }

    auto iter2 = ht.find(300);
    EXPECT_NE(iter, ht.end());
    auto next_iter2 = ht.erase(iter2);
    EXPECT_EQ(ht.size(), 7);
    EXPECT_EQ(ht.find(300), ht.end());
    ht.print_structure();

    // 验证返回的迭代器指向下一个有效元素或end()
    if (next_iter2 != ht.end())
    {
        EXPECT_NE(next_iter2->key, 300);
        EXPECT_EQ(next_iter2->key, 700);
    }
}

// 迭代器测试
TEST(NFShmHashTableTest, Iterator)
{
    TestHashTable ht;

    // 插入测试数据
    std::vector<int> keys = {1, 5, 3, 8, 2, 7, 4, 6};
    for (int key : keys)
    {
        TestPair pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    // 测试迭代器遍历
    std::set<int> found_keys;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        found_keys.insert(iter->key);
    }

    EXPECT_EQ(found_keys.size(), keys.size());
    for (int key : keys)
    {
        EXPECT_TRUE(found_keys.count(key) > 0);
    }

    // 测试const迭代器
    const TestHashTable& const_ht = ht;
    ht.print_structure();
    const_ht.print_structure();
    std::set<int> const_found_keys;
    for (auto iter = const_ht.begin(); iter != const_ht.end(); ++iter)
    {
        const_found_keys.insert(iter->key);
    }

    EXPECT_EQ(const_found_keys, found_keys);
}

// 容量测试
TEST(NFShmHashTableTest, Capacity)
{
    TestHashTable ht;

    // 填满哈希表
    for (int i = 0; i < 100; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        auto result = ht.insert_unique(pair);
        EXPECT_TRUE(result.second);
    }

    EXPECT_EQ(ht.size(), 100);
    EXPECT_TRUE(ht.full());
    EXPECT_EQ(ht.left_size(), 0);

    // 尝试再插入一个元素，应该失败
    TestPair extra_pair(100, "extra");
    auto result = ht.insert_unique(extra_pair);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(ht.size(), 100);
}

// equal_range测试
TEST(NFShmHashTableTest, EqualRange)
{
    TestHashTable ht;

    // 插入相同键的多个值
    TestPair pair1(1, "value1");
    TestPair pair2(1, "value2");
    TestPair pair3(1, "value3");
    TestPair pair4(2, "value4");

    ht.insert_equal(pair1);
    ht.insert_equal(pair2);
    ht.insert_equal(pair3);
    ht.insert_equal(pair4);

    // 测试equal_range
    auto range = ht.equal_range(1);
    int count = 0;
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        EXPECT_EQ(iter->key, 1);
        count++;
    }
    EXPECT_EQ(count, 3);

    // 测试不存在的键
    auto empty_range = ht.equal_range(999);
    EXPECT_EQ(empty_range.first, ht.end());
    EXPECT_EQ(empty_range.second, ht.end());
}

// find_or_insert测试
TEST(NFShmHashTableTest, FindOrInsert)
{
    TestHashTable ht;

    // 第一次调用应该插入
    TestPair pair1(1, "value1");
    auto& ref1 = ht.find_or_insert(pair1);
    EXPECT_EQ(ref1.key, 1);
    EXPECT_EQ(ref1.value, "value1");
    EXPECT_EQ(ht.size(), 1);

    // 第二次调用应该找到现有的
    TestPair pair2(1, "value2");
    auto& ref2 = ht.find_or_insert(pair2);
    EXPECT_EQ(ref2.key, 1);
    EXPECT_EQ(ref2.value, "value1"); // 应该是原来的值
    EXPECT_EQ(ht.size(), 1);

    // 验证引用指向同一个对象
    EXPECT_EQ(&ref1, &ref2);
}

// 复制构造和赋值测试
TEST(NFShmHashTableTest, CopyAndAssignment)
{
    TestHashTable ht1;

    // 插入测试数据
    for (int i = 1; i <= 5; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht1.insert_unique(pair);
    }

    // 测试复制构造
    TestHashTable ht2(ht1);
    EXPECT_EQ(ht2.size(), ht1.size());

    for (int i = 1; i <= 5; ++i)
    {
        auto iter1 = ht1.find(i);
        auto iter2 = ht2.find(i);
        EXPECT_NE(iter1, ht1.end());
        EXPECT_NE(iter2, ht2.end());
        EXPECT_EQ(iter1->key, iter2->key);
        EXPECT_EQ(iter1->value, iter2->value);
    }

    // 测试赋值操作
    TestHashTable ht3;
    ht3 = ht1;
    EXPECT_EQ(ht3.size(), ht1.size());

    for (int i = 1; i <= 5; ++i)
    {
        auto iter1 = ht1.find(i);
        auto iter3 = ht3.find(i);
        EXPECT_NE(iter1, ht1.end());
        EXPECT_NE(iter3, ht3.end());
        EXPECT_EQ(iter1->key, iter3->key);
        EXPECT_EQ(iter1->value, iter3->value);
    }
}

// swap测试
TEST(NFShmHashTableTest, Swap)
{
    TestHashTable ht1, ht2;

    // 在ht1中插入数据
    for (int i = 1; i <= 3; ++i)
    {
        TestPair pair(i, "ht1_value" + std::to_string(i));
        ht1.insert_unique(pair);
    }

    // 在ht2中插入数据
    for (int i = 4; i <= 6; ++i)
    {
        TestPair pair(i, "ht2_value" + std::to_string(i));
        ht2.insert_unique(pair);
    }

    size_t size1 = ht1.size();
    size_t size2 = ht2.size();

    // 执行swap
    ht1.swap(ht2);

    // 验证大小交换
    EXPECT_EQ(ht1.size(), size2);
    EXPECT_EQ(ht2.size(), size1);

    // 验证内容交换
    for (int i = 4; i <= 6; ++i)
    {
        auto iter = ht1.find(i);
        EXPECT_NE(iter, ht1.end());
        EXPECT_EQ(iter->value, "ht2_value" + std::to_string(i));
    }

    for (int i = 1; i <= 3; ++i)
    {
        auto iter = ht2.find(i);
        EXPECT_NE(iter, ht2.end());
        EXPECT_EQ(iter->value, "ht1_value" + std::to_string(i));
    }
}

// clear测试
TEST(NFShmHashTableTest, Clear)
{
    TestHashTable ht;

    // 插入测试数据
    for (int i = 1; i <= 10; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    EXPECT_EQ(ht.size(), 10);
    EXPECT_FALSE(ht.empty());

    // 清空
    ht.clear();

    EXPECT_EQ(ht.size(), 0);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.left_size(), 100);

    // 验证所有元素都被删除
    for (int i = 1; i <= 10; ++i)
    {
        EXPECT_EQ(ht.find(i), ht.end());
    }
}

// bucket相关测试
TEST(NFShmHashTableTest, BucketOperations)
{
    TestHashTable ht;

    // 插入一些数据
    for (int i = 0; i < 20; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    // 测试bucket计数
    size_t total_elements = 0;
    for (size_t i = 0; i < ht.bucket_count(); ++i)
    {
        total_elements += ht.elems_in_bucket(i);
    }

    EXPECT_EQ(total_elements, ht.size());
}

// 比较操作符测试
TEST(NFShmHashTableTest, ComparisonOperators)
{
    TestHashTable ht1, ht2;

    // 两个空哈希表应该相等
    EXPECT_TRUE(ht1 == ht2);
    EXPECT_FALSE(ht1 != ht2);

    // 插入相同数据
    for (int i = 1; i <= 5; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht1.insert_unique(pair);
        ht2.insert_unique(pair);
    }

    EXPECT_TRUE(ht1 == ht2);
    EXPECT_FALSE(ht1 != ht2);

    // 插入不同数据
    TestPair extra_pair(6, "value6");
    ht1.insert_unique(extra_pair);

    EXPECT_FALSE(ht1 == ht2);
    EXPECT_TRUE(ht1 != ht2);
}

// 范围插入测试
TEST(NFShmHashTableTest, RangeInsert)
{
    TestHashTable ht;

    // 准备测试数据
    std::vector<TestPair> test_data;
    for (int i = 1; i <= 10; ++i)
    {
        test_data.emplace_back(i, "value" + std::to_string(i));
    }

    // 测试范围插入unique
    ht.insert_unique(test_data.begin(), test_data.end());
    EXPECT_EQ(ht.size(), 10);

    // 验证所有元素都被插入
    for (int i = 1; i <= 10; ++i)
    {
        auto iter = ht.find(i);
        EXPECT_NE(iter, ht.end());
        EXPECT_EQ(iter->value, "value" + std::to_string(i));
    }

    // 测试范围插入equal
    TestHashTable ht2;
    ht2.insert_equal(test_data.begin(), test_data.end());
    EXPECT_EQ(ht2.size(), 10);
}

// 打印函数测试（主要测试不会崩溃）
TEST(NFShmHashTableTest, PrintFunctions)
{
    TestHashTable ht;

    // 插入一些测试数据
    for (int i = 1; i <= 5; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    // 测试打印函数（主要确保不会崩溃）
    testing::internal::CaptureStdout();
    ht.print_simple();
    std::string simple_output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(simple_output.empty());

    testing::internal::CaptureStdout();
    ht.print_structure();
    std::string structure_output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(structure_output.empty());

    testing::internal::CaptureStdout();
    ht.print_detailed();
    std::string detailed_output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(detailed_output.empty());
}

// 边界条件测试
TEST(NFShmHashTableTest, EdgeCases)
{
    TestHashTable ht;

    // 测试空哈希表的操作
    EXPECT_EQ(ht.begin(), ht.end());
    EXPECT_EQ(ht.count(1), 0);
    EXPECT_EQ(ht.erase(1), 0);
    EXPECT_EQ(ht.find(1), ht.end());

    // 测试单个元素
    TestPair pair(1, "value1");
    ht.insert_unique(pair);

    auto iter = ht.begin();
    EXPECT_NE(iter, ht.end());
    ++iter;
    EXPECT_EQ(iter, ht.end());

    // 删除唯一元素
    ht.erase(1);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.begin(), ht.end());
}

// 迭代器安全性测试
TEST(NFShmHashTableTest, IteratorSafety)
{
    TestHashTable ht;

    // 插入测试数据
    for (int i = 1; i <= 10; ++i)
    {
        TestPair pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    // 测试迭代器在删除操作后的行为
    auto iter = ht.find(5);
    EXPECT_NE(iter, ht.end());

    auto next_iter = ht.erase(iter);
    // next_iter应该指向下一个有效元素或end()
    if (next_iter != ht.end())
    {
        EXPECT_NE(next_iter->key, 5);
    }

    // 验证删除成功
    EXPECT_EQ(ht.find(5), ht.end());
    EXPECT_EQ(ht.size(), 9);
}
