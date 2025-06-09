// -------------------------------------------------------------------------
//    @FileName         :    TestNFShmRBTreeWithList.h
//    @Author           :    Craft
//    @Date             :    2024/12/19
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmRBTreeWithList Test
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFShmStl/NFShmRBTreeWithList.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <set>

// 测试用的简单Key-Value结构
struct TestKeyValueWithList
{
    int key;
    std::string value;

    TestKeyValueWithList() : key(0), value("")
    {
    }

    TestKeyValueWithList(int k, const std::string& v) : key(k), value(v)
    {
    }

    TestKeyValueWithList(const TestKeyValueWithList& other)
    {
        if (this != &other)
        {
            key = other.key;
            value = other.value;
        }
    }

    bool operator==(const TestKeyValueWithList& other) const
    {
        return key == other.key && value == other.value;
    }
};

// 键值提取器 - 用于从TestKeyValueWithList中提取key
struct KeyOfValueWithList
{
    const int& operator()(const TestKeyValueWithList& kv) const
    {
        return kv.key;
    }
};

// 用于测试的红黑树类型 - 最大容量为100个节点
typedef NFShmRBTreeWithList<int, TestKeyValueWithList, KeyOfValueWithList, 100> TestRBTreeWithList;

// ==================== 基础功能测试 ====================

class NFRBTreeWithListBasicTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试树初始化和基本状态
TEST_F(NFRBTreeWithListBasicTest, InitializationTest)
{
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
    EXPECT_EQ(tree.max_size(), 100);
    EXPECT_FALSE(tree.full());
    EXPECT_EQ(tree.begin(), tree.end());
    EXPECT_EQ(tree.list_begin(), tree.list_end());
    
    // 测试LRU默认状态
    EXPECT_FALSE(tree.is_lru_enabled());
}

// 测试唯一键插入操作
TEST_F(NFRBTreeWithListBasicTest, InsertUniqueTest)
{
    auto result1 = tree.insert_unique(TestKeyValueWithList(1, "one"));
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->key, 1);
    EXPECT_EQ(result1.first->value, "one");
    EXPECT_EQ(tree.size(), 1);

    auto result2 = tree.insert_unique(TestKeyValueWithList(2, "two"));
    EXPECT_TRUE(result2.second);
    EXPECT_EQ(tree.size(), 2);

    // 测试插入已存在的键
    auto result3 = tree.insert_unique(TestKeyValueWithList(1, "another one"));
    EXPECT_FALSE(result3.second);
    EXPECT_EQ(result3.first->value, "one");
    EXPECT_EQ(tree.size(), 2);
    
    EXPECT_TRUE(tree.RbVerify());
}

// 测试允许重复键的插入操作
TEST_F(NFRBTreeWithListBasicTest, InsertEqualTest)
{
    auto it1 = tree.insert_equal(TestKeyValueWithList(1, "one"));
    EXPECT_EQ(it1->key, 1);
    EXPECT_EQ(tree.size(), 1);

    auto it2 = tree.insert_equal(TestKeyValueWithList(1, "another one"));
    EXPECT_EQ(it2->key, 1);
    EXPECT_EQ(tree.size(), 2);

    EXPECT_TRUE(tree.RbVerify());
}

// 测试查找操作
TEST_F(NFRBTreeWithListBasicTest, FindTest)
{
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));
    tree.insert_unique(TestKeyValueWithList(3, "three"));

    auto it1 = tree.find(1);
    EXPECT_NE(it1, tree.end());
    EXPECT_EQ(it1->value, "one");

    auto it2 = tree.find(2);
    EXPECT_NE(it2, tree.end());
    EXPECT_EQ(it2->value, "two");

    auto it3 = tree.find(4);
    EXPECT_EQ(it3, tree.end());
}

// 测试删除操作
TEST_F(NFRBTreeWithListBasicTest, EraseTest)
{
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));
    tree.insert_unique(TestKeyValueWithList(3, "three"));
    EXPECT_EQ(tree.size(), 3);

    auto it = tree.find(2);
    tree.erase(it);
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(tree.find(2), tree.end());

    size_t count = tree.erase(1);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(tree.find(1), tree.end());

    EXPECT_TRUE(tree.RbVerify());
}

// ==================== 链表功能测试 ====================

class NFRBTreeWithListListTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试插入顺序维护
TEST_F(NFRBTreeWithListListTest, InsertionOrderTest)
{
    // 插入一些元素（注意插入顺序和红黑树顺序不同）
    tree.insert_unique(TestKeyValueWithList(3, "three"));
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(4, "four"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));

    // 红黑树遍历应该是有序的 (1, 2, 3, 4)
    std::vector<int> tree_order;
    for (auto it = tree.begin(); it != tree.end(); ++it)
    {
        tree_order.push_back(it->key);
    }
    EXPECT_EQ(tree_order, std::vector<int>({1, 2, 3, 4}));

    // 链表遍历应该按插入顺序 (3, 1, 4, 2)
    std::vector<int> list_order;
    for (auto it = tree.list_begin(); it != tree.list_end(); ++it)
    {
        list_order.push_back(it->key);
    }
    EXPECT_EQ(list_order, std::vector<int>({3, 1, 4, 2}));
}

// 测试链表反向遍历
TEST_F(NFRBTreeWithListListTest, ReverseListIterationTest)
{
    tree.insert_unique(TestKeyValueWithList(3, "three"));
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(4, "four"));

    // 反向遍历应该是最新插入的元素在前
    std::vector<int> reverse_order;
    for (auto it = tree.list_rbegin(); it != tree.list_rend(); ++it)
    {
        reverse_order.push_back(it->key);
    }
    // 应该是 (4, 1, 3) - 按插入顺序的逆序
    EXPECT_EQ(reverse_order, std::vector<int>({4, 1, 3}));
}

// 测试删除对链表的影响
TEST_F(NFRBTreeWithListListTest, EraseFromListTest)
{
    tree.insert_unique(TestKeyValueWithList(3, "three"));
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(4, "four"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));

    // 删除中间的元素
    tree.erase(1);

    // 链表遍历应该跳过被删除的元素
    std::vector<int> list_order;
    for (auto it = tree.list_begin(); it != tree.list_end(); ++it)
    {
        list_order.push_back(it->key);
    }
    EXPECT_EQ(list_order, std::vector<int>({3, 4, 2}));
}

// ==================== LRU功能测试 ====================

class NFRBTreeWithListLRUTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
        tree.enable_lru(); // 启用LRU功能
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试LRU功能控制
TEST_F(NFRBTreeWithListLRUTest, LRUControlTest)
{
    EXPECT_TRUE(tree.is_lru_enabled());
    
    tree.disable_lru();
    EXPECT_FALSE(tree.is_lru_enabled());
    
    tree.enable_lru();
    EXPECT_TRUE(tree.is_lru_enabled());
}

// 测试find操作的LRU移动
TEST_F(NFRBTreeWithListLRUTest, FindLRUTest)
{
    // 插入一些元素
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));
    tree.insert_unique(TestKeyValueWithList(3, "three"));

    // 初始插入顺序: 1, 2, 3
    std::vector<int> initial_order;
    for (auto it = tree.list_begin(); it != tree.list_end(); ++it)
    {
        initial_order.push_back(it->key);
    }
    EXPECT_EQ(initial_order, std::vector<int>({1, 2, 3}));

    // 查找元素1，应该将其移动到链表尾部
    auto found = tree.find(1);
    EXPECT_NE(found, tree.end());

    // 链表顺序应该变为: 2, 3, 1
    std::vector<int> after_find_order;
    for (auto it = tree.list_begin(); it != tree.list_end(); ++it)
    {
        after_find_order.push_back(it->key);
    }
    EXPECT_EQ(after_find_order, std::vector<int>({2, 3, 1}));
}

// 测试count操作的LRU移动
TEST_F(NFRBTreeWithListLRUTest, CountLRUTest)
{
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));
    tree.insert_unique(TestKeyValueWithList(3, "three"));

    // 调用count，应该触发LRU移动
    size_t count = tree.count(2);
    EXPECT_EQ(count, 1);

    // 元素2应该被移动到链表尾部
    std::vector<int> after_count_order;
    for (auto it = tree.list_begin(); it != tree.list_end(); ++it)
    {
        after_count_order.push_back(it->key);
    }
    EXPECT_EQ(after_count_order, std::vector<int>({1, 3, 2}));
}

// ==================== 批量操作测试 ====================

class NFRBTreeWithListBatchTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试批量唯一插入
TEST_F(NFRBTreeWithListBatchTest, BatchInsertUniqueTest)
{
    std::vector<TestKeyValueWithList> test_data;
    for (int i = 1; i <= 10; ++i)
    {
        test_data.emplace_back(i, "value_" + std::to_string(i));
    }

    // 使用迭代器范围插入
    tree.insert_unique(test_data.begin(), test_data.end());

    EXPECT_EQ(tree.size(), 10);
    
    // 验证所有元素都插入成功
    for (int i = 1; i <= 10; ++i)
    {
        auto it = tree.find(i);
        EXPECT_NE(it, tree.end());
        EXPECT_EQ(it->value, "value_" + std::to_string(i));
    }

    EXPECT_TRUE(tree.RbVerify());
}

// 测试批量等值插入
TEST_F(NFRBTreeWithListBatchTest, BatchInsertEqualTest)
{
    std::vector<TestKeyValueWithList> test_data;
    test_data.emplace_back(1, "first");
    test_data.emplace_back(1, "second");
    test_data.emplace_back(2, "third");
    test_data.emplace_back(1, "fourth");

    tree.insert_equal(test_data.begin(), test_data.end());

    EXPECT_EQ(tree.size(), 4);
    EXPECT_EQ(tree.count(1), 3); // 键1有3个值
    EXPECT_EQ(tree.count(2), 1); // 键2有1个值

    EXPECT_TRUE(tree.RbVerify());
}

// 测试范围查询
TEST_F(NFRBTreeWithListBatchTest, RangeQueryTest)
{
    // 插入测试数据
    for (int i = 1; i <= 10; ++i)
    {
        tree.insert_unique(TestKeyValueWithList(i, "value_" + std::to_string(i)));
    }

    // 测试lower_bound和upper_bound
    auto lower = tree.lower_bound(3);
    auto upper = tree.upper_bound(7);

    std::vector<int> range_keys;
    for (auto it = lower; it != upper; ++it)
    {
        range_keys.push_back(it->key);
    }

    EXPECT_EQ(range_keys, std::vector<int>({3, 4, 5, 6, 7}));

    // 测试equal_range
    auto range = tree.equal_range(5);
    EXPECT_NE(range.first, tree.end());
    EXPECT_EQ(range.first->key, 5);
    EXPECT_EQ(std::distance(range.first, range.second), 1);
}

// ==================== 压力测试 ====================

class NFRBTreeWithListStressTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试大量随机插入删除
TEST_F(NFRBTreeWithListStressTest, RandomInsertDeleteTest)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(1, 1000);

    std::set<int> reference_set; // 用于对比验证

    // 随机插入50个元素
    for (int i = 0; i < 50; ++i)
    {
        int key = dis(gen);
        tree.insert_unique(TestKeyValueWithList(key, "value_" + std::to_string(key)));
        reference_set.insert(key);
    }

    // 验证树的完整性
    EXPECT_TRUE(tree.RbVerify());
    EXPECT_EQ(tree.size(), reference_set.size());

    // 验证所有元素都能找到
    for (int key : reference_set)
    {
        EXPECT_NE(tree.find(key), tree.end());
    }

    // 随机删除一半元素
    auto it = reference_set.begin();
    std::advance(it, reference_set.size() / 2);
    
    for (auto del_it = reference_set.begin(); del_it != it; ++del_it)
    {
        tree.erase(*del_it);
    }

    // 验证删除后的状态
    EXPECT_TRUE(tree.RbVerify());
    
    // 验证被删除的元素找不到，剩余元素能找到
    for (auto check_it = reference_set.begin(); check_it != reference_set.end(); ++check_it)
    {
        if (check_it == it) break;
        EXPECT_EQ(tree.find(*check_it), tree.end()); // 被删除的元素
    }
    
    for (auto check_it = it; check_it != reference_set.end(); ++check_it)
    {
        EXPECT_NE(tree.find(*check_it), tree.end()); // 剩余的元素
    }
}

// 测试容量限制
TEST_F(NFRBTreeWithListStressTest, CapacityLimitTest)
{
    // 尝试插入超过最大容量的元素
    for (int i = 1; i <= 110; ++i) // 超过100的容量限制
    {
        auto result = tree.insert_unique(TestKeyValueWithList(i, "value_" + std::to_string(i)));
        if (i <= 100)
        {
            EXPECT_TRUE(result.second); // 前100个应该成功
        }
        else
        {
            EXPECT_FALSE(result.second); // 超过容量的应该失败
        }
    }

    EXPECT_EQ(tree.size(), 100);
    EXPECT_TRUE(tree.full());
    EXPECT_TRUE(tree.RbVerify());
}

// ==================== 打印功能测试 ====================

class NFRBTreeWithListPrintTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
        // 插入一些测试数据
        tree.insert_unique(TestKeyValueWithList(5, "five"));
        tree.insert_unique(TestKeyValueWithList(3, "three"));
        tree.insert_unique(TestKeyValueWithList(7, "seven"));
        tree.insert_unique(TestKeyValueWithList(1, "one"));
        tree.insert_unique(TestKeyValueWithList(9, "nine"));
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试打印功能（这里主要确保不会崩溃）
TEST_F(NFRBTreeWithListPrintTest, PrintFunctionsTest)
{
    // 这些函数主要是为了调试，确保它们不会崩溃即可
    EXPECT_NO_THROW(tree.print_structure());
    EXPECT_NO_THROW(tree.print_detailed());
    EXPECT_NO_THROW(tree.print_simple());
}

// ==================== 边界条件测试 ====================

class NFRBTreeWithListEdgeCaseTest : public testing::Test
{
protected:
    void SetUp() override
    {
        tree.clear();
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTreeWithList tree;
};

// 测试空树操作
TEST_F(NFRBTreeWithListEdgeCaseTest, EmptyTreeTest)
{
    EXPECT_EQ(tree.find(1), tree.end());
    EXPECT_EQ(tree.count(1), 0);
    EXPECT_EQ(tree.erase(1), 0);
    EXPECT_EQ(tree.lower_bound(1), tree.end());
    EXPECT_EQ(tree.upper_bound(1), tree.end());

    auto range = tree.equal_range(1);
    EXPECT_EQ(range.first, tree.end());
    EXPECT_EQ(range.second, tree.end());
}

// 测试单元素操作
TEST_F(NFRBTreeWithListEdgeCaseTest, SingleElementTest)
{
    tree.insert_unique(TestKeyValueWithList(42, "answer"));

    EXPECT_EQ(tree.size(), 1);
    EXPECT_FALSE(tree.empty());
    
    auto it = tree.find(42);
    EXPECT_NE(it, tree.end());
    EXPECT_EQ(it->key, 42);

    // 测试链表迭代器
    auto list_it = tree.list_begin();
    EXPECT_NE(list_it, tree.list_end());
    EXPECT_EQ(list_it->key, 42);
    ++list_it;
    EXPECT_EQ(list_it, tree.list_end());

    // 删除唯一元素
    tree.erase(42);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.list_begin(), tree.list_end());
}

// 测试迭代器操作
TEST_F(NFRBTreeWithListEdgeCaseTest, IteratorTest)
{
    tree.insert_unique(TestKeyValueWithList(1, "one"));
    tree.insert_unique(TestKeyValueWithList(2, "two"));
    tree.insert_unique(TestKeyValueWithList(3, "three"));

    // 测试前向迭代
    auto it = tree.begin();
    EXPECT_EQ(it->key, 1);
    ++it;
    EXPECT_EQ(it->key, 2);
    ++it;
    EXPECT_EQ(it->key, 3);
    ++it;
    EXPECT_EQ(it, tree.end());

    // 测试后向迭代
    it = tree.end();
    --it;
    EXPECT_EQ(it->key, 3);
    --it;
    EXPECT_EQ(it->key, 2);
    --it;
    EXPECT_EQ(it->key, 1);

    // 测试链表迭代器
    auto list_it = tree.list_begin();
    EXPECT_EQ(list_it->key, 1); // 按插入顺序
    ++list_it;
    EXPECT_EQ(list_it->key, 2);
    ++list_it;
    EXPECT_EQ(list_it->key, 3);
    ++list_it;
    EXPECT_EQ(list_it, tree.list_end());
} 