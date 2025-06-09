// -------------------------------------------------------------------------
//    @FileName         :    TestRBTree.h
//    @Author           :    Craft
//    @Date             :    2023/10/20
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmRBTree Test
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFShmStl/NFShmRBTree.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

// 测试用的简单Key-Value结构
struct TestKeyValue
{
    int key;
    std::string value;

    TestKeyValue() : key(0), value("")
    {
    }

    TestKeyValue(int k, const std::string& v) : key(k), value(v)
    {
    }

    TestKeyValue(const TestKeyValue& other)
    {
        if (this != &other)
        {
            key = other.key;
            value = other.value;
        }
    }

    bool operator==(const TestKeyValue& other) const
    {
        return key == other.key && value == other.value;
    }
};

// 键值提取器 - 用于从TestKeyValue中提取key
struct KeyOfValue
{
    const int& operator()(const TestKeyValue& kv) const
    {
        return kv.key;
    }
};

// 用于测试的红黑树类型 - 最大容量为100个节点
typedef NFShmRBTree<int, TestKeyValue, KeyOfValue, 100> TestRBTree;

// 红黑树基本功能测试类
class NFRBTreeBasicTest : public testing::Test
{
protected:
    // 每个测试前的设置
    void SetUp() override
    {
    }

    // 每个测试后的清理
    void TearDown() override
    {
        // 清空树
        tree.clear();
    }

    TestRBTree tree;
};

// 测试树初始化和基本状态
TEST_F(NFRBTreeBasicTest, InitializationTest)
{
    // 验证新创建的树是空的
    EXPECT_TRUE(tree.empty());
    // 验证树的大小为0
    EXPECT_EQ(tree.size(), 0);
    // 验证树的最大容量为100
    EXPECT_EQ(tree.max_size(), 100);
    // 验证树不是满的
    EXPECT_FALSE(tree.full());
    // 验证begin和end迭代器相等（因为树是空的）
    EXPECT_EQ(tree.begin(), tree.end());
}

// 测试唯一键插入操作 - 不允许重复键
TEST_F(NFRBTreeBasicTest, InsertUniqueTest)
{
    // 插入第一个元素
    auto result1 = tree.insert_unique(TestKeyValue(1, "one"));
    // 验证插入成功
    EXPECT_TRUE(result1.second);
    // 验证插入元素的键值
    EXPECT_EQ(result1.first->key, 1);
    EXPECT_EQ(result1.first->value, "one");
    // 验证树大小增加到1
    EXPECT_EQ(tree.size(), 1);

    // 插入第二个元素
    auto result2 = tree.insert_unique(TestKeyValue(2, "two"));
    EXPECT_TRUE(result2.second);
    // 验证树大小增加到2
    EXPECT_EQ(tree.size(), 2);

    // 测试插入已存在的键
    auto result3 = tree.insert_unique(TestKeyValue(1, "another one"));
    // 验证插入失败（因为键1已存在）
    EXPECT_FALSE(result3.second);
    // 验证返回的是原有元素
    EXPECT_EQ(result3.first->value, "one");
    // 验证树大小不变
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(tree.RbVerify(), true);

    tree.print_structure();
    tree.print_simple();
    tree.print_detailed();
}

// 测试允许重复键的插入操作
TEST_F(NFRBTreeBasicTest, InsertEqualTest)
{
    // 插入第一个元素
    auto it1 = tree.insert_equal(TestKeyValue(1, "one"));
    EXPECT_EQ(it1->key, 1);
    EXPECT_EQ(tree.size(), 1);

    // 插入重复键元素
    auto it2 = tree.insert_equal(TestKeyValue(1, "another one"));
    EXPECT_EQ(it2->key, 1);
    // 验证树大小增加到2
    EXPECT_EQ(tree.size(), 2);

    // 检查迭代器遍历的顺序
    auto it = tree.begin();
    EXPECT_EQ(it->key, 1);
    EXPECT_EQ(it->value, "one");
    ++it;
    EXPECT_EQ(it->key, 1);
    EXPECT_EQ(it->value, "another one");
    EXPECT_EQ(tree.RbVerify(), true);
}

// 测试查找操作
TEST_F(NFRBTreeBasicTest, FindTest)
{
    // 插入一些测试数据
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(2, "two"));
    tree.insert_unique(TestKeyValue(3, "three"));

    // 查找存在的键
    auto it1 = tree.find(1);
    // 验证找到的元素不是end()
    EXPECT_NE(it1, tree.end());
    // 验证找到元素的值
    EXPECT_EQ(it1->value, "one");

    auto it2 = tree.find(2);
    EXPECT_NE(it2, tree.end());
    EXPECT_EQ(it2->value, "two");

    // 查找不存在的键
    auto it3 = tree.find(4);
    // 验证返回end()迭代器
    EXPECT_EQ(it3, tree.end());
}

// 测试删除操作
TEST_F(NFRBTreeBasicTest, EraseTest)
{
    // 插入一些测试数据
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(2, "two"));
    tree.insert_unique(TestKeyValue(3, "three"));
    EXPECT_EQ(tree.size(), 3);

    // 使用迭代器删除
    auto it = tree.find(2);
    tree.erase(it);
    // 验证树大小减少
    EXPECT_EQ(tree.size(), 2);
    // 验证元素2已被删除
    EXPECT_EQ(tree.find(2), tree.end());

    // 使用键删除
    size_t count = tree.erase(1);
    // 验证删除了1个元素
    EXPECT_EQ(count, 1);
    EXPECT_EQ(tree.size(), 1);
    // 验证元素1已被删除
    EXPECT_EQ(tree.find(1), tree.end());

    // 删除不存在的键
    count = tree.erase(5);
    // 验证没有删除任何元素
    EXPECT_EQ(count, 0);
    // 验证树大小不变
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(tree.RbVerify(), true);
}

// 测试迭代器操作
TEST_F(NFRBTreeBasicTest, IteratorTest)
{
    // 插入有序数据
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(2, "two"));
    tree.insert_unique(TestKeyValue(3, "three"));

    // 使用正向迭代器遍历并检查顺序
    auto it = tree.begin();
    EXPECT_EQ(it->key, 1);
    ++it;
    EXPECT_EQ(it->key, 2);
    ++it;
    EXPECT_EQ(it->key, 3);
    ++it;
    EXPECT_EQ(it, tree.end());

    // 测试反向迭代器遍历
    auto rit = tree.rbegin();
    EXPECT_EQ(rit->key, 3);
    ++rit;
    EXPECT_EQ(rit->key, 2);
    ++rit;
    EXPECT_EQ(rit->key, 1);
    ++rit;
    EXPECT_EQ(rit, tree.rend());
}

// 测试边界查找操作
TEST_F(NFRBTreeBasicTest, BoundaryTest)
{
    // 插入一些值
    tree.insert_equal(TestKeyValue(1, "one"));
    tree.insert_equal(TestKeyValue(3, "three"));
    tree.insert_equal(TestKeyValue(5, "five"));
    tree.insert_equal(TestKeyValue(3, "another three"));

    // 测试lower_bound - 返回不小于指定键的第一个元素
    auto it1 = tree.lower_bound(2);
    // 验证第一个不小于2的元素是3
    EXPECT_EQ(it1->key, 3);

    auto it2 = tree.lower_bound(3);
    // 验证第一个不小于3的元素是3
    EXPECT_EQ(it2->key, 3);

    auto it3 = tree.lower_bound(6);
    // 验证没有不小于6的元素
    EXPECT_EQ(it3, tree.end());

    // 测试upper_bound - 返回大于指定键的第一个元素
    auto it4 = tree.upper_bound(2);
    // 验证第一个大于2的元素是3
    EXPECT_EQ(it4->key, 3);

    auto it5 = tree.upper_bound(3);
    // 验证第一个大于3的元素是5
    EXPECT_EQ(it5->key, 5);

    auto it6 = tree.upper_bound(5);
    // 验证没有大于5的元素
    EXPECT_EQ(it6, tree.end());

    // 测试equal_range - 返回等于指定键的元素范围
    auto range1 = tree.equal_range(3);
    EXPECT_EQ(range1.first->key, 3);
    EXPECT_EQ(range1.second->key, 5);
    // 验证有2个键为3的元素
    EXPECT_EQ(std::distance(range1.first, range1.second), 2);
    EXPECT_EQ(tree.RbVerify(), true);
}

// 测试计数操作
TEST_F(NFRBTreeBasicTest, CountTest)
{
    // 测试空树
    EXPECT_EQ(tree.count(1), 0);

    // 使用insert_unique插入不重复元素
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(2, "two"));

    // 验证每个键只有1个元素
    EXPECT_EQ(tree.count(1), 1);
    EXPECT_EQ(tree.count(2), 1);
    EXPECT_EQ(tree.count(3), 0);

    // 清空树并使用insert_equal插入重复元素
    tree.clear();
    tree.insert_equal(TestKeyValue(1, "one"));
    tree.insert_equal(TestKeyValue(1, "another one"));
    tree.insert_equal(TestKeyValue(2, "two"));

    // 验证键1有2个元素，键2有1个元素
    EXPECT_EQ(tree.count(1), 2);
    EXPECT_EQ(tree.count(2), 1);
}

// 测试emplace操作（原地构造）
TEST_F(NFRBTreeBasicTest, EmplaceTest)
{
    // 测试emplace_unique - 不允许重复键
    auto result1 = tree.emplace_unique(1, "one");
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->key, 1);
    EXPECT_EQ(result1.first->value, "one");

    // 尝试插入已存在的键
    auto result2 = tree.emplace_unique(1, "another one");
    // 验证插入失败
    EXPECT_FALSE(result2.second);

    // 测试emplace_equal - 允许重复键
    auto it1 = tree.emplace_equal(2, "two");
    EXPECT_EQ(it1->key, 2);

    // 插入重复键
    auto it2 = tree.emplace_equal(2, "another two");
    EXPECT_EQ(it2->key, 2);
    // 验证键2有2个元素
    EXPECT_EQ(tree.count(2), 2);
}

// 测试交换操作
TEST_F(NFRBTreeBasicTest, SwapTest)
{
    // 准备两棵树
    TestRBTree tree2;
    tree2.CreateInit();

    // 在第一棵树中插入元素
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(2, "two"));

    // 在第二棵树中插入元素
    tree2.insert_unique(TestKeyValue(3, "three"));
    tree2.insert_unique(TestKeyValue(4, "four"));

    // 交换前验证树的状态
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(tree2.size(), 2);
    EXPECT_NE(tree.find(1), tree.end());
    EXPECT_EQ(tree.find(3), tree.end());
    EXPECT_NE(tree2.find(3), tree2.end());
    EXPECT_EQ(tree2.find(1), tree2.end());

    // 交换两棵树
    tree.swap(tree2);

    // 交换后验证树的状态
    EXPECT_EQ(tree.size(), 2);
    EXPECT_EQ(tree2.size(), 2);
    EXPECT_EQ(tree.find(1), tree.end());
    EXPECT_NE(tree.find(3), tree.end());
    EXPECT_EQ(tree2.find(3), tree2.end());
    EXPECT_NE(tree2.find(1), tree2.end());
    EXPECT_EQ(tree.RbVerify(), true);
}

// 红黑树压力测试类
class NFRBTreeStressTest : public testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
        tree.clear();
    }

    TestRBTree tree;
};

// 大量数据随机插入和删除测试
TEST_F(NFRBTreeStressTest, RandomInsertDeleteTest)
{
    // 创建随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);

    // 测试数据量（不超过树的最大容量）
    const int TEST_SIZE = 900;
    NFShmRBTree<int, TestKeyValue, KeyOfValue, TEST_SIZE>* pMaxTree = new NFShmRBTree<int, TestKeyValue, KeyOfValue, TEST_SIZE>();
    std::vector<int> keys;

    // 随机生成键并插入树中
    for (int i = 0; i < TEST_SIZE;)
    {
        do
        {
            int key = dis(gen);
            std::string value = "value_" + std::to_string(key);
            auto result = pMaxTree->insert_unique(TestKeyValue(key, value));
            if (result.second)
            {
                keys.push_back(key);
                ++i;
                break;
            }
            EXPECT_EQ(pMaxTree->__rb_verify(), true);
        } while (true);
    }

    // 验证所有插入的键都可以在树中找到
    for (int key : keys)
    {
        EXPECT_GT(pMaxTree->count(key), 0);
    }

    // 随机打乱键的顺序，然后删除一半
    std::shuffle(keys.begin(), keys.end(), gen);
    for (int i = 0; i < TEST_SIZE / 2; ++i)
    {
        pMaxTree->erase(keys[i]);
        // 验证删除后找不到该键
        EXPECT_EQ(pMaxTree->count(keys[i]), 0);
        EXPECT_EQ(pMaxTree->__rb_verify(), true);
    }

    // 验证未删除的键仍然存在
    for (int i = TEST_SIZE / 2; i < TEST_SIZE; ++i)
    {
        EXPECT_GT(pMaxTree->count(keys[i]), 0);
    }
    EXPECT_EQ(pMaxTree->__rb_verify(), true);
    delete pMaxTree;
}

// 测试树的有序性
TEST_F(NFRBTreeStressTest, OrderingTest)
{
    // 插入一些无序的数据
    std::vector<int> keys = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    for (int key : keys)
    {
        tree.insert_unique(TestKeyValue(key, "value_" + std::to_string(key)));
    }

    // 收集树遍历的所有键值
    std::vector<int> orderedKeys;
    for (auto it = tree.begin(); it != tree.end(); ++it)
    {
        orderedKeys.push_back(it->key);
    }

    // 预期的有序序列
    std::vector<int> expectedOrder = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    // 验证树遍历结果是有序的
    EXPECT_EQ(orderedKeys, expectedOrder);
    EXPECT_EQ(tree.RbVerify(), true);
}

// 测试树的满容量
TEST_F(NFRBTreeStressTest, FullCapacityTest)
{
    // 填充到接近最大容量
    for (int i = 0; i < 99; ++i)
    {
        tree.insert_unique(TestKeyValue(i, "value_" + std::to_string(i)));
    }

    // 验证树几乎已满但还未满
    EXPECT_EQ(tree.size(), 99);
    EXPECT_FALSE(tree.full());

    // 再插入一个元素，达到最大容量
    tree.insert_unique(TestKeyValue(99, "value_99"));
    EXPECT_EQ(tree.size(), 100);
    EXPECT_TRUE(tree.full());

    // 再次插入应该失败（树已满）
    auto result = tree.insert_unique(TestKeyValue(100, "value_100"));
    EXPECT_FALSE(result.second);
    EXPECT_EQ(tree.size(), 100); // 大小不变
    EXPECT_EQ(tree.RbVerify(), true);
}

// 多种操作混合测试
TEST_F(NFRBTreeStressTest, MixedOperationsTest)
{
    // 执行各种操作的混合
    tree.insert_unique(TestKeyValue(1, "one"));
    tree.insert_unique(TestKeyValue(3, "three"));

    // 插入和查找
    auto result = tree.insert_unique(TestKeyValue(2, "two"));
    EXPECT_TRUE(result.second);
    EXPECT_EQ(tree.find(2)->value, "two");

    // 删除和插入等值
    tree.erase(1);
    EXPECT_EQ(tree.find(1), tree.end());
    tree.insert_equal(TestKeyValue(3, "another three"));
    EXPECT_EQ(tree.count(3), 2);

    // 范围操作
    auto range = tree.equal_range(3);
    EXPECT_EQ(std::distance(range.first, range.second), 2);

    // 迭代和修改元素值
    for (auto it = tree.begin(); it != tree.end(); ++it)
    {
        if (it->key == 2)
        {
            // 注意：直接修改迭代器指向的数据应该谨慎，这里仅作测试
            const_cast<TestKeyValue&>(*it).value = "modified two";
        }
    }

    // 验证修改成功
    EXPECT_EQ(tree.find(2)->value, "modified two");
    EXPECT_EQ(tree.RbVerify(), true);
}



