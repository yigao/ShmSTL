// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmHashTableWithList.h
//    @Author           :    gaoyi
//    @Date             :    2025/1/27
//    @Email            :    445267987@qq.com
//    @Module           :    TestNFShmHashTableWithList
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmHashTableWithList.h"
#include <string>
#include <functional>
#include <vector>
#include <set>

// 测试用的简单键值对结构
struct TestPairWithList
{
    int key;
    std::string value;

    TestPairWithList() : key(0), value("")
    {
    }

    TestPairWithList(int k, const std::string& v) : key(k), value(v)
    {
    }

    bool operator==(const TestPairWithList& other) const
    {
        return key == other.key && value == other.value;
    }
};

// 提取键的函数对象
struct ExtractKeyWithList
{
    int operator()(const TestPairWithList& pair) const
    {
        return pair.key;
    }
};

// 哈希函数对象
struct HashFuncWithList
{
    size_t operator()(int key) const
    {
        return std::hash<int>{}(key);
    }
};

// 键比较函数对象
struct EqualKeyWithList
{
    bool operator()(int key1, int key2) const
    {
        return key1 == key2;
    }
};

// 定义测试用的哈希表类型
using TestHashTableWithList = NFShmHashTableWithList<TestPairWithList, int, 50, HashFuncWithList, ExtractKeyWithList, EqualKeyWithList>;

// ==================== 基础链表功能测试 ====================

TEST(NFShmHashTableWithListTest, BasicListConstruction)
{
    TestHashTableWithList ht;

    EXPECT_EQ(ht.size(), 0);
    EXPECT_EQ(ht.max_size(), 50);
    EXPECT_TRUE(ht.empty());
    EXPECT_FALSE(ht.full());
    EXPECT_FALSE(ht.is_lru_enabled()); // 默认应该禁用LRU

    // 测试空链表迭代器
    EXPECT_EQ(ht.list_begin(), ht.list_end());
    EXPECT_EQ(ht.list_cbegin(), ht.list_cend());
}

TEST(NFShmHashTableWithListTest, InsertionOrderMaintenance)
{
    TestHashTableWithList ht;

    // 按特定顺序插入元素
    std::vector<int> insert_order = {5, 1, 8, 3, 2, 7, 4, 6};
    for (int key : insert_order)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        auto result = ht.insert_unique(pair);
        EXPECT_TRUE(result.second);
    }

    ht.print_structure();

    EXPECT_EQ(ht.size(), insert_order.size());

    // 验证链表迭代器按插入顺序遍历
    auto list_iter = ht.list_begin();
    for (size_t i = 0; i < insert_order.size(); ++i)
    {
        EXPECT_NE(list_iter, ht.list_end());
        EXPECT_EQ(list_iter->key, insert_order[i]);
        EXPECT_EQ(list_iter->value, "value" + std::to_string(insert_order[i]));
        ++list_iter;
    }
    EXPECT_EQ(list_iter, ht.list_end());
}

TEST(NFShmHashTableWithListTest, ListIteratorTraversal)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {10, 20, 30, 40, 50};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    // 测试非常量迭代器
    std::vector<int> traversed_keys;
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        traversed_keys.push_back(iter->key);
    }
    EXPECT_EQ(traversed_keys, keys);

    // 测试常量迭代器
    traversed_keys.clear();
    const auto& const_ht = ht;
    for (auto iter = const_ht.list_begin(); iter != const_ht.list_end(); ++iter)
    {
        traversed_keys.push_back(iter->key);
    }
    EXPECT_EQ(traversed_keys, keys);

    // 测试C++11风格常量迭代器
    traversed_keys.clear();
    for (auto iter = ht.list_cbegin(); iter != ht.list_cend(); ++iter)
    {
        traversed_keys.push_back(iter->key);
    }
    EXPECT_EQ(traversed_keys, keys);
}

TEST(NFShmHashTableWithListTest, ListAfterErase)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {1, 2, 3, 4, 5};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    // 删除中间元素
    ht.erase(3);

    // 验证链表顺序：应该是 1, 2, 4, 5
    std::vector<int> expected = {1, 2, 4, 5};
    std::vector<int> actual;
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        actual.push_back(iter->key);
    }
    EXPECT_EQ(actual, expected);

    // 删除头元素
    ht.erase(1);
    expected = {2, 4, 5};
    actual.clear();
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        actual.push_back(iter->key);
    }
    EXPECT_EQ(actual, expected);

    // 删除尾元素
    ht.erase(5);
    expected = {2, 4};
    actual.clear();
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        actual.push_back(iter->key);
    }
    EXPECT_EQ(actual, expected);
}

// ==================== LRU功能测试 ====================

TEST(NFShmHashTableWithListTest, LRUEnableDisable)
{
    TestHashTableWithList ht;

    // 默认应该禁用LRU
    EXPECT_FALSE(ht.is_lru_enabled());

    // 启用LRU
    ht.enable_lru();
    EXPECT_TRUE(ht.is_lru_enabled());

    // 禁用LRU
    ht.disable_lru();
    EXPECT_FALSE(ht.is_lru_enabled());
}

TEST(NFShmHashTableWithListTest, LRUFindBehavior)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {1, 2, 3, 4, 5};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    // 在LRU禁用时，find不应该改变顺序
    ht.disable_lru();
    auto iter = ht.find(2);
    EXPECT_NE(iter, ht.end());

    // 验证顺序没有改变
    std::vector<int> order_after_find;
    for (auto list_iter = ht.list_begin(); list_iter != ht.list_end(); ++list_iter)
    {
        order_after_find.push_back(list_iter->key);
    }
    EXPECT_EQ(order_after_find, keys); // 顺序应该不变

    // 启用LRU后，find应该将元素移动到尾部
    ht.enable_lru();
    iter = ht.find(2);
    EXPECT_NE(iter, ht.end());

    // 验证2被移动到了尾部，新顺序应该是 1, 3, 4, 5, 2
    std::vector<int> expected_order = {1, 3, 4, 5, 2};
    order_after_find.clear();
    for (auto list_iter = ht.list_begin(); list_iter != ht.list_end(); ++list_iter)
    {
        order_after_find.push_back(list_iter->key);
    }
    EXPECT_EQ(order_after_find, expected_order);
}

TEST(NFShmHashTableWithListTest, LRUCountBehavior)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {10, 20, 30, 40, 50};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ht.enable_lru();

    // 使用count访问元素30
    size_t count = ht.count(30);
    EXPECT_EQ(count, 1);

    // 验证30被移动到了尾部
    std::vector<int> expected_order = {10, 20, 40, 50, 30};
    std::vector<int> actual_order;
    for (auto list_iter = ht.list_begin(); list_iter != ht.list_end(); ++list_iter)
    {
        actual_order.push_back(list_iter->key);
    }
    EXPECT_EQ(actual_order, expected_order);
}

TEST(NFShmHashTableWithListTest, LRUMultipleAccesses)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {1, 2, 3, 4, 5};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ht.enable_lru();

    // 按顺序访问：2, 4, 1
    ht.find(2); // 顺序变为: 1, 3, 4, 5, 2
    ht.find(4); // 顺序变为: 1, 3, 5, 2, 4
    ht.find(1); // 顺序变为: 3, 5, 2, 4, 1

    std::vector<int> expected_order = {3, 5, 2, 4, 1};
    std::vector<int> actual_order;
    for (auto list_iter = ht.list_begin(); list_iter != ht.list_end(); ++list_iter)
    {
        actual_order.push_back(list_iter->key);
    }
    EXPECT_EQ(actual_order, expected_order);
}

TEST(NFShmHashTableWithListTest, LRUAccessNonexistentKey)
{
    TestHashTableWithList ht;

    // 插入测试数据
    std::vector<int> keys = {1, 2, 3};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }

    ht.enable_lru();

    // 访问不存在的键，不应该改变顺序
    auto iter = ht.find(999);
    EXPECT_EQ(iter, ht.end());

    std::vector<int> actual_order;
    for (auto list_iter = ht.list_begin(); list_iter != ht.list_end(); ++list_iter)
    {
        actual_order.push_back(list_iter->key);
    }
    EXPECT_EQ(actual_order, keys); // 顺序应该保持不变
}

// ==================== 边界情况测试 ====================

TEST(NFShmHashTableWithListTest, SingleElementList)
{
    TestHashTableWithList ht;

    // 插入单个元素
    TestPairWithList pair(42, "answer");
    ht.insert_unique(pair);

    // 验证链表迭代器
    auto iter = ht.list_begin();
    EXPECT_NE(iter, ht.list_end());
    EXPECT_EQ(iter->key, 42);
    EXPECT_EQ(iter->value, "answer");

    ++iter;
    EXPECT_EQ(iter, ht.list_end());

    // 测试LRU访问单个元素
    ht.enable_lru();
    auto find_iter = ht.find(42);
    EXPECT_NE(find_iter, ht.end());

    // 单个元素的LRU访问不应该改变任何东西
    iter = ht.list_begin();
    EXPECT_EQ(iter->key, 42);
    ++iter;
    EXPECT_EQ(iter, ht.list_end());
}

TEST(NFShmHashTableWithListTest, ClearAndReset)
{
    TestHashTableWithList ht;

    // 插入数据并启用LRU
    for (int i = 1; i <= 10; ++i)
    {
        TestPairWithList pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }
    ht.enable_lru();

    // 清空哈希表
    ht.clear();

    EXPECT_EQ(ht.size(), 0);
    EXPECT_TRUE(ht.empty());
    EXPECT_EQ(ht.list_begin(), ht.list_end());

    // LRU设置应该保持
    EXPECT_TRUE(ht.is_lru_enabled());

    // 重新插入数据
    TestPairWithList pair(100, "new_value");
    ht.insert_unique(pair);

    auto iter = ht.list_begin();
    EXPECT_NE(iter, ht.list_end());
    EXPECT_EQ(iter->key, 100);
    ++iter;
    EXPECT_EQ(iter, ht.list_end());
}

TEST(NFShmHashTableWithListTest, InsertEqual)
{
    TestHashTableWithList ht;

    // 插入重复键的元素
    TestPairWithList pair1(1, "first");
    TestPairWithList pair2(1, "second");
    TestPairWithList pair3(1, "third");

    ht.insert_equal(pair1);
    ht.insert_equal(pair2);
    ht.insert_equal(pair3);

    EXPECT_EQ(ht.size(), 3);
    EXPECT_EQ(ht.count(1), 3);

    // 验证插入顺序维护
    std::vector<std::string> expected_values = {"first", "second", "third"};
    std::vector<std::string> actual_values;

    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        if (iter->key == 1)
        {
            actual_values.push_back(iter->value);
        }
    }

    EXPECT_EQ(actual_values, expected_values);
}

// ==================== 复制和赋值测试 ====================

TEST(NFShmHashTableWithListTest, CopyConstructor)
{
    TestHashTableWithList ht1;

    // 在ht1中插入数据并设置LRU
    std::vector<int> keys = {1, 2, 3, 4, 5};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht1.insert_unique(pair);
    }
    ht1.enable_lru();

    // 访问一些元素改变LRU顺序
    ht1.find(3);
    ht1.find(1);

    // 复制构造
    TestHashTableWithList ht2(ht1);

    // 验证大小和LRU设置
    EXPECT_EQ(ht2.size(), ht1.size());
    EXPECT_EQ(ht2.is_lru_enabled(), ht1.is_lru_enabled());

    // 验证插入顺序一致
    std::vector<int> order1, order2;
    for (auto iter = ht1.list_begin(); iter != ht1.list_end(); ++iter)
    {
        order1.push_back(iter->key);
    }
    for (auto iter = ht2.list_begin(); iter != ht2.list_end(); ++iter)
    {
        order2.push_back(iter->key);
    }
    EXPECT_EQ(order1, order2);
}

TEST(NFShmHashTableWithListTest, AssignmentOperator)
{
    TestHashTableWithList ht1, ht2;

    // 在ht1中插入数据
    for (int i = 10; i <= 50; i += 10)
    {
        TestPairWithList pair(i, "value" + std::to_string(i));
        ht1.insert_unique(pair);
    }
    ht1.enable_lru();

    // 在ht2中插入不同的数据
    for (int i = 1; i <= 3; ++i)
    {
        TestPairWithList pair(i, "old_value" + std::to_string(i));
        ht2.insert_unique(pair);
    }

    // 赋值操作
    ht2 = ht1;

    // 验证ht2现在与ht1相同
    EXPECT_EQ(ht2.size(), ht1.size());
    EXPECT_EQ(ht2.is_lru_enabled(), ht1.is_lru_enabled());

    // 验证链表顺序一致
    auto iter1 = ht1.list_begin();
    auto iter2 = ht2.list_begin();
    while (iter1 != ht1.list_end() && iter2 != ht2.list_end())
    {
        EXPECT_EQ(iter1->key, iter2->key);
        EXPECT_EQ(iter1->value, iter2->value);
        ++iter1;
        ++iter2;
    }
    EXPECT_EQ(iter1, ht1.list_end());
    EXPECT_EQ(iter2, ht2.list_end());
}

// ==================== 性能和容量测试 ====================

TEST(NFShmHashTableWithListTest, FullCapacityInsertion)
{
    TestHashTableWithList ht;

    // 填满哈希表
    for (int i = 1; i <= 50; ++i)
    {
        TestPairWithList pair(i, "value" + std::to_string(i));
        auto result = ht.insert_unique(pair);
        EXPECT_TRUE(result.second);
    }

    EXPECT_EQ(ht.size(), 50);
    EXPECT_TRUE(ht.full());
    EXPECT_EQ(ht.left_size(), 0);

    // 验证所有元素都在链表中
    std::set<int> expected_keys;
    for (int i = 1; i <= 50; ++i)
    {
        expected_keys.insert(i);
    }

    std::set<int> actual_keys;
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        actual_keys.insert(iter->key);
    }

    EXPECT_EQ(actual_keys, expected_keys);

    // 尝试插入更多元素应该失败
    TestPairWithList extra_pair(51, "extra");
    auto result = ht.insert_unique(extra_pair);
    EXPECT_FALSE(result.second);
    EXPECT_EQ(ht.size(), 50);
}

// ==================== 打印功能测试 ====================

TEST(NFShmHashTableWithListTest, PrintFunctions)
{
    TestHashTableWithList ht;

    // 插入一些数据
    std::vector<int> keys = {15, 25, 35};
    for (int key : keys)
    {
        TestPairWithList pair(key, "value" + std::to_string(key));
        ht.insert_unique(pair);
    }
    ht.enable_lru();

    // 这些函数主要用于调试，我们只是确保它们不会崩溃
    EXPECT_NO_THROW(ht.print_structure());
    EXPECT_NO_THROW(ht.print_list());
    EXPECT_NO_THROW(ht.print_detailed());
    EXPECT_NO_THROW(ht.print_simple());

    // 测试空表的打印
    TestHashTableWithList empty_ht;
    EXPECT_NO_THROW(empty_ht.print_structure());
    EXPECT_NO_THROW(empty_ht.print_list());
}

// ==================== 链表完整性测试 ====================

TEST(NFShmHashTableWithListTest, ListIntegrityAfterOperations)
{
    TestHashTableWithList ht;

    // 插入、删除、再插入的混合操作
    for (int i = 1; i <= 10; ++i)
    {
        TestPairWithList pair(i, "value" + std::to_string(i));
        ht.insert_unique(pair);
    }

    // 删除一些元素
    ht.erase(3);
    ht.erase(7);
    ht.erase(1);

    // 再插入一些新元素
    TestPairWithList pair11(11, "value11");
    TestPairWithList pair12(12, "value12");
    ht.insert_unique(pair11);
    ht.insert_unique(pair12);

    // 验证链表的完整性
    std::vector<int> expected_keys;
    for (auto iter = ht.begin(); iter != ht.end(); ++iter)
    {
        expected_keys.push_back(iter->key);
    }
    std::sort(expected_keys.begin(), expected_keys.end());

    std::vector<int> list_keys;
    for (auto iter = ht.list_begin(); iter != ht.list_end(); ++iter)
    {
        list_keys.push_back(iter->key);
    }
    std::sort(list_keys.begin(), list_keys.end());

    // 链表中的键应该与哈希表中的键完全一致
    EXPECT_EQ(list_keys, expected_keys);
    EXPECT_EQ(ht.size(), list_keys.size());
}

// ==================== equal_range LRU功能测试 ====================

TEST(NFShmHashTableWithListTest, EqualRangeLRUBehavior)
{
    TestHashTableWithList ht;

    // 插入一些测试数据，包括重复的键
    std::vector<int> keys = {1, 2, 1, 3, 2, 4, 1}; // 键1有3个，键2有2个
    for (size_t i = 0; i < keys.size(); ++i)
    {
        TestPairWithList pair(keys[i], "value" + std::to_string(i));
        ht.insert_equal(pair);
    }

    EXPECT_EQ(ht.size(), 7);

    // 启用LRU功能
    ht.enable_lru();
    EXPECT_TRUE(ht.is_lru_enabled());

    // 记录查询前的插入顺序
    std::vector<int> order_before_query;
    for (auto it = ht.list_begin(); it != ht.list_end(); ++it)
    {
        order_before_query.push_back(it->key);
    }

    printf("\n=== equal_range LRU Test ===\n");
    printf("Before equal_range query - insertion order: ");
    for (int k : order_before_query)
    {
        printf("%d ", k);
    }
    printf("\n");

    // 使用 equal_range 查询键1（应该有3个匹配元素）
    auto range = ht.equal_range(1);

    // 验证找到的元素
    int count = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->key, 1);
        count++;
    }
    EXPECT_GT(count, 0); // 应该找到至少一个元素

    // 记录查询后的插入顺序
    std::vector<int> order_after_query;
    for (auto it = ht.list_begin(); it != ht.list_end(); ++it)
    {
        order_after_query.push_back(it->key);
    }

    printf("After equal_range query - insertion order: ");
    for (int k : order_after_query)
    {
        printf("%d ", k);
    }
    printf("\n");

    // 验证LRU行为：被查询的键1的元素应该移动到链表尾部
    // 统计链表尾部连续的键1元素数量
    int trailing_ones = 0;
    for (auto it = ht.list_begin(); it != ht.list_end(); ++it)
    {
        if (it->key == 1)
        {
            // 找到第一个键1，计算从这里到尾部有多少个键1
            auto temp_it = it;
            int consecutive_ones = 0;
            while (temp_it != ht.list_end() && temp_it->key == 1)
            {
                consecutive_ones++;
                ++temp_it;
            }
            if (temp_it == ht.list_end()) // 到达了链表尾部
            {
                trailing_ones = consecutive_ones;
            }
            break;
        }
    }

    printf("Trailing consecutive key=1 elements: %d\n", trailing_ones);

    // 验证至少有一些键1的元素被移到了尾部
    EXPECT_GT(trailing_ones, 0);

    // 测试常量版本的equal_range
    const auto& const_ht = ht;
    auto const_range = const_ht.equal_range(2);

    // 验证常量版本也能找到元素
    int const_count = 0;
    for (auto it = const_range.first; it != const_range.second; ++it)
    {
        EXPECT_EQ(it->key, 2);
        const_count++;
    }
    EXPECT_GT(const_count, 0);

    // 禁用LRU并再次测试
    ht.disable_lru();
    EXPECT_FALSE(ht.is_lru_enabled());

    // 记录禁用LRU前的顺序
    std::vector<int> order_before_disabled;
    for (auto it = ht.list_begin(); it != ht.list_end(); ++it)
    {
        order_before_disabled.push_back(it->key);
    }

    // 查询键3（只有1个）
    auto range_disabled = ht.equal_range(3);

    // 记录禁用LRU后的顺序
    std::vector<int> order_after_disabled;
    for (auto it = ht.list_begin(); it != ht.list_end(); ++it)
    {
        order_after_disabled.push_back(it->key);
    }

    // 禁用LRU后，顺序应该保持不变
    EXPECT_EQ(order_before_disabled, order_after_disabled);

    printf("LRU disabled - order unchanged: %s\n",
           (order_before_disabled == order_after_disabled) ? "Yes" : "No");
    printf("========================\n");
}
