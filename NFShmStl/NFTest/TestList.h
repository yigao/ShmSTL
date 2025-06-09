// -------------------------------------------------------------------------
//    @FileName         :    TestVector.h
//    @Author           :    gaoyi
//    @Date             :    2025/4/25
//    @Email            :    445267987@qq.com
//    @Module           :    TestVector
//
// -------------------------------------------------------------------------

#pragma once

#include <gtest/gtest.h>
#include "NFComm/NFShmStl/NFShmList.h"
#include <string>
#include <memory>

// 自定义测试类
class TestClass
{
public:
    TestClass() : value(0), name("default")
    {
    }

    TestClass(int v, const std::string& n) : value(v), name(n)
    {
    }

    bool operator==(const TestClass& other) const
    {
        return value == other.value && name == other.name;
    }

    bool operator<(const TestClass& other) const
    {
        return value < other.value;
    }

    int value;
    std::string name;
};

class NFShmListTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 每个测试用例前都会执行
    }

    void TearDown() override
    {
        // 每个测试用例后都会执行
    }
};

// 基本功能测试 - 使用int类型
TEST_F(NFShmListTest, BasicOperationsWithInt)
{
    NFShmList<int, 10> list;

    // 测试空列表
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);

    // 测试push_back
    list.push_back(1);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(list.back(), 1);

    // 测试push_front
    list.push_front(0);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), 0);
    EXPECT_EQ(list.back(), 1);
}

// 使用string类型的测试
TEST_F(NFShmListTest, StringOperations)
{
    NFShmList<std::string, 5> list;

    // 测试字符串操作
    list.push_back("Hello");
    list.push_back("World");
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front(), "Hello");
    EXPECT_EQ(list.back(), "World");

    // 测试字符串拼接
    list.push_front("Say");
    EXPECT_EQ(list.front(), "Say");

    // 测试字符串比较
    list.push_back("!");
    EXPECT_EQ(list.back(), "!");

    // 测试字符串查找
    auto it = std::find(list.begin(), list.end(), "World");
    EXPECT_NE(it, list.end());
    EXPECT_EQ(*it, "World");
}

// 使用自定义类的测试
TEST_F(NFShmListTest, CustomClassOperations)
{
    NFShmList<TestClass, 5> list;

    // 测试自定义类操作
    TestClass obj1(1, "First");
    TestClass obj2(2, "Second");

    list.push_back(obj1);
    list.push_back(obj2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front().value, 1);
    EXPECT_EQ(list.front().name, "First");
    EXPECT_EQ(list.back().value, 2);
    EXPECT_EQ(list.back().name, "Second");

    // 测试自定义类比较
    TestClass obj3(1, "First");
    list.push_back(obj3);
    EXPECT_EQ(list.size(), 3);

    // 测试自定义类排序
    list.sort();
    EXPECT_EQ(list.front().value, 1);
    EXPECT_EQ(list.back().value, 2);
}

// 使用智能指针的测试
TEST_F(NFShmListTest, SmartPointerOperations)
{
    NFShmList<std::shared_ptr<TestClass>, 5> list;

    // 测试智能指针操作
    auto ptr1 = std::make_shared<TestClass>(1, "First");
    auto ptr2 = std::make_shared<TestClass>(2, "Second");

    list.push_back(ptr1);
    list.push_back(ptr2);
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.front()->value, 1);
    EXPECT_EQ(list.back()->value, 2);

    // 测试智能指针引用计数
    auto ptr3 = ptr1;
    EXPECT_EQ(ptr1.use_count(), 3);
}

// 边界测试 - 使用不同数据类型
TEST_F(NFShmListTest, BoundaryTestsWithDifferentTypes)
{
    // 测试int类型边界
    NFShmList<int, 2> intList;
    intList.push_back(1);
    intList.push_back(2);
    EXPECT_TRUE(intList.full());
    intList.push_back(3); // 应该失败
    EXPECT_EQ(intList.size(), 2);

    // 测试string类型边界
    NFShmList<std::string, 2> strList;
    strList.push_back("First");
    strList.push_back("Second");
    EXPECT_TRUE(strList.full());
    strList.push_back("Third"); // 应该失败
    EXPECT_EQ(strList.size(), 2);

    // 测试自定义类型边界
    NFShmList<TestClass, 2> classList;
    classList.push_back(TestClass(1, "First"));
    classList.push_back(TestClass(2, "Second"));
    EXPECT_TRUE(classList.full());
    classList.push_back(TestClass(3, "Third")); // 应该失败
    EXPECT_EQ(classList.size(), 2);
}

// 迭代器测试 - 使用不同数据类型
TEST_F(NFShmListTest, IteratorTestsWithDifferentTypes)
{
    // 测试int类型迭代
    NFShmList<int, 5> intList;
    for (int i = 0; i < 3; i++)
    {
        intList.push_back(i);
    }
    int expected = 0;
    for (auto it = intList.begin(); it != intList.end(); ++it)
    {
        EXPECT_EQ(*it, expected++);
    }

    // 测试string类型迭代
    NFShmList<std::string, 5> strList;
    strList.push_back("A");
    strList.push_back("B");
    strList.push_back("C");
    std::string expectedStr = "A";
    for (auto it = strList.begin(); it != strList.end(); ++it)
    {
        EXPECT_EQ(*it, expectedStr);
        expectedStr[0]++;
    }

    // 测试自定义类型迭代
    NFShmList<TestClass, 5> classList;
    for (int i = 0; i < 3; i++)
    {
        classList.push_back(TestClass(i, "Test" + std::to_string(i)));
    }
    expected = 0;
    for (auto it = classList.begin(); it != classList.end(); ++it)
    {
        EXPECT_EQ(it->value, expected++);
    }
}

// 特殊操作测试 - 使用不同数据类型
TEST_F(NFShmListTest, SpecialOperationsWithDifferentTypes)
{
    // 测试int类型特殊操作
    NFShmList<int, 10> intList;
    intList.assign(3, 1);
    EXPECT_EQ(intList.size(), 3);
    for (auto it = intList.begin(); it != intList.end(); ++it)
    {
        EXPECT_EQ(*it, 1);
    }

    // 测试string类型特殊操作
    NFShmList<std::string, 10> strList;
    strList.assign(3, "Test");
    EXPECT_EQ(strList.size(), 3);
    for (auto it = strList.begin(); it != strList.end(); ++it)
    {
        EXPECT_EQ(*it, "Test");
    }

    // 测试自定义类型特殊操作
    NFShmList<TestClass, 10> classList;
    classList.assign(3, TestClass(1, "Test"));
    EXPECT_EQ(classList.size(), 3);
    for (auto it = classList.begin(); it != classList.end(); ++it)
    {
        EXPECT_EQ(it->value, 1);
        EXPECT_EQ(it->name, "Test");
    }
}

// 性能测试 - 使用不同数据类型
TEST_F(NFShmListTest, PerformanceTestsWithDifferentTypes)
{
    // 测试int类型性能
    NFShmList<int, 1000> intList;
    for (int i = 0; i < 1000; i++)
    {
        intList.push_back(i);
    }
    EXPECT_EQ(intList.size(), 1000);

    // 测试string类型性能
    NFShmList<std::string, 1000> strList;
    for (int i = 0; i < 1000; i++)
    {
        strList.push_back("Test" + std::to_string(i));
    }
    EXPECT_EQ(strList.size(), 1000);

    // 测试自定义类型性能
    NFShmList<TestClass, 1000> classList;
    for (int i = 0; i < 1000; i++)
    {
        classList.push_back(TestClass(i, "Test" + std::to_string(i)));
    }
    EXPECT_EQ(classList.size(), 1000);
}

// 复制和移动测试 - 使用不同数据类型
TEST_F(NFShmListTest, CopyAndMoveTestsWithDifferentTypes)
{
    // 测试int类型复制
    NFShmList<int, 5> intList1;
    intList1.push_back(1);
    intList1.push_back(2);
    NFShmList<int, 5> intList2(intList1);
    EXPECT_EQ(intList2.size(), 2);
    EXPECT_EQ(intList2.front(), 1);
    EXPECT_EQ(intList2.back(), 2);

    // 测试string类型复制
    NFShmList<std::string, 5> strList1;
    strList1.push_back("First");
    strList1.push_back("Second");
    NFShmList<std::string, 5> strList2(strList1);
    EXPECT_EQ(strList2.size(), 2);
    EXPECT_EQ(strList2.front(), "First");
    EXPECT_EQ(strList2.back(), "Second");

    // 测试自定义类型复制
    NFShmList<TestClass, 5> classList1;
    classList1.push_back(TestClass(1, "First"));
    classList1.push_back(TestClass(2, "Second"));
    NFShmList<TestClass, 5> classList2(classList1);
    EXPECT_EQ(classList2.size(), 2);
    EXPECT_EQ(classList2.front().value, 1);
    EXPECT_EQ(classList2.back().value, 2);
}

// 更多基本操作测试
TEST_F(NFShmListTest, MoreBasicOperations)
{
    NFShmList<int, 10> list;

    // 测试clear
    list.push_back(1);
    list.push_back(2);
    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);

    // 测试resize
    list.resize(5, 10);
    EXPECT_EQ(list.size(), 5);
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        EXPECT_EQ(*it, 10);
    }

    // 测试pop_front和pop_back
    list.clear();
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.pop_front();
    EXPECT_EQ(list.front(), 2);
    list.pop_back();
    EXPECT_EQ(list.back(), 2);
}

// 越界测试
TEST_F(NFShmListTest, OutOfBoundsTests)
{
    NFShmList<int, 3> list;

    // 测试空列表访问
    EXPECT_EQ(&list.front(), &list.m_staticError);
    EXPECT_EQ(&list.back(), &list.m_staticError);

    // 测试迭代器越界
    auto it = list.begin();
    EXPECT_EQ(++it, list.end());

    // 测试pop空列表
    EXPECT_NO_THROW(list.pop_front());
    EXPECT_NO_THROW(list.pop_back());

    // 测试resize越界
    EXPECT_NO_THROW(list.resize(5));
}

// 迭代器操作测试
TEST_F(NFShmListTest, IteratorOperations)
{
    NFShmList<int, 5> list;
    for (int i = 0; i < 3; i++)
    {
        list.push_back(i);
    }

    // 测试迭代器自增自减
    auto it = list.begin();
    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 1);
    --it;
    EXPECT_EQ(*it, 0);

    // 测试迭代器比较
    auto it1 = list.begin();
    auto it2 = list.begin();
    EXPECT_TRUE(it1 == it2);
    ++it2;
    EXPECT_TRUE(it1 != it2);

    // 测试迭代器赋值
    it1 = it2;
    EXPECT_EQ(*it1, 1);
}

// 元素访问测试
TEST_F(NFShmListTest, ElementAccess)
{
    NFShmList<int, 5> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    // 测试front和back
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(list.back(), 3);

    // 测试迭代器访问
    auto it = list.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
}

// 列表操作测试
TEST_F(NFShmListTest, ListOperations)
{
    NFShmList<int, 10> list1;
    NFShmList<int, 10> list2;

    // 测试assign
    list1.assign(3, 5);
    EXPECT_EQ(list1.size(), 3);
    for (auto it = list1.begin(); it != list1.end(); ++it)
    {
        EXPECT_EQ(*it, 5);
    }

    // 测试swap
    list1.clear();
    list2.clear();
    list1.push_back(1);
    list2.push_back(2);
    list1.swap(list2);
    EXPECT_EQ(list1.front(), 2);
    EXPECT_EQ(list2.front(), 1);

    // 测试remove
    list1.clear();
    list1.push_back(1);
    list1.push_back(2);
    list1.push_back(1);
    list1.remove(1);
    EXPECT_EQ(list1.size(), 1);
    EXPECT_EQ(list1.front(), 2);
}

// 性能测试 - 大量数据操作
TEST_F(NFShmListTest, LargeDataOperations)
{
    NFShmList<int, 10000> list;

    // 测试大量push_back
    for (int i = 0; i < 10000; i++)
    {
        list.push_back(i);
    }
    EXPECT_EQ(list.size(), 10000);

    // 测试大量pop_front
    for (int i = 0; i < 5000; i++)
    {
        list.pop_front();
    }
    EXPECT_EQ(list.size(), 5000);

    // 测试大量pop_back
    for (int i = 0; i < 5000; i++)
    {
        list.pop_back();
    }
    EXPECT_TRUE(list.empty());
}

// 特殊值测试
TEST_F(NFShmListTest, SpecialValueTests)
{
    NFShmList<int, 5> list;

    // 测试0值
    list.push_back(0);
    EXPECT_EQ(list.front(), 0);

    // 测试负值
    list.push_back(-1);
    EXPECT_EQ(list.back(), -1);

    // 测试最大值
    list.push_back(INT_MAX);
    EXPECT_EQ(list.back(), INT_MAX);

    // 测试最小值
    list.push_back(INT_MIN);
    EXPECT_EQ(list.back(), INT_MIN);
}

// 字符串特殊测试
TEST_F(NFShmListTest, StringSpecialTests)
{
    NFShmList<std::string, 5> list;

    // 测试空字符串
    list.push_back("");
    EXPECT_EQ(list.front(), "");

    // 测试长字符串
    std::string longStr(1000, 'a');
    list.push_back(longStr);
    EXPECT_EQ(list.back(), longStr);

    // 测试特殊字符
    list.push_back("\n\t\r");
    EXPECT_EQ(list.back(), "\n\t\r");

    // 测试Unicode字符
    list.push_back("你好世界");
    EXPECT_EQ(list.back(), "你好世界");
}

// 自定义类特殊测试
TEST_F(NFShmListTest, CustomClassSpecialTests)
{
    NFShmList<TestClass, 5> list;

    // 测试默认构造
    list.push_back(TestClass());
    EXPECT_EQ(list.front().value, 0);
    EXPECT_EQ(list.front().name, "default");

    // 测试复制构造
    TestClass obj(1, "test");
    list.push_back(obj);
    EXPECT_EQ(list.back().value, 1);
    EXPECT_EQ(list.back().name, "test");

    // 测试移动构造
    list.push_back(TestClass(2, "move"));
    EXPECT_EQ(list.back().value, 2);
    EXPECT_EQ(list.back().name, "move");
}

// 迭代器特殊测试
TEST_F(NFShmListTest, IteratorSpecialTests)
{
    NFShmList<int, 5> list;
    for (int i = 0; i < 3; i++)
    {
        list.push_back(i);
    }

    // 测试迭代器范围
    auto begin = list.begin();
    auto end = list.end();
    int count = 0;
    for (auto it = begin; it != end; ++it)
    {
        count++;
    }
    EXPECT_EQ(count, 3);

    // 测试迭代器距离
    EXPECT_EQ(std::distance(begin, end), 3);

    // 测试迭代器交换
    auto it1 = list.begin();
    auto it2 = ++list.begin();
    std::swap(*it1, *it2);
    EXPECT_EQ(list.front(), 1);
    EXPECT_EQ(*(++list.begin()), 0);
}

// 异常情况测试
TEST_F(NFShmListTest, ExceptionTests)
{
    NFShmList<int, 3> list;

    // 测试空列表操作
    EXPECT_NO_THROW(list.pop_front());
    EXPECT_NO_THROW(list.pop_back());

    // 测试越界访问
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    auto it = list.end();
    EXPECT_EQ(++it, list.begin());

    // 测试无效迭代器
    NFShmList<int, 3> list2;
    auto it2 = list2.begin();
    EXPECT_NO_THROW(*it2);
}

// 内存测试
TEST_F(NFShmListTest, MemoryTests)
{
    NFShmList<int, 1000> list;

    // 测试内存分配
    for (int i = 0; i < 1000; i++)
    {
        list.push_back(i);
    }
    EXPECT_EQ(list.size(), 1000);

    // 测试内存释放
    list.clear();
    EXPECT_TRUE(list.empty());

    // 测试内存重用
    for (int i = 0; i < 1000; i++)
    {
        list.push_back(i);
    }
    EXPECT_EQ(list.size(), 1000);
}

// 排序和查找测试
TEST_F(NFShmListTest, SortAndFindTests)
{
    NFShmList<int, 100> list;

    // 测试排序
    for (int i = 0; i < 10; i++)
    {
        list.push_back(9 - i);
    }
    list.sort();
    int expected = 0;
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        EXPECT_EQ(*it, expected++);
    }

    // 测试查找
    auto it = std::find(list.begin(), list.end(), 5);
    EXPECT_NE(it, list.end());
    EXPECT_EQ(*it, 5);

    // 测试二分查找
    list.clear();
    for (int i = 0; i < 10; i++)
    {
        list.push_back(i * 2);
    }
    auto lower = std::lower_bound(list.begin(), list.end(), 5);
    EXPECT_EQ(*lower, 6);
}

// 列表高级操作测试
TEST_F(NFShmListTest, AdvancedListOperations)
{
    // splice测试
    NFShmList<int, 15> list1;
    NFShmList<int, 15> list2;

    // 准备数据
    for (int i = 0; i < 5; i++)
    {
        list1.push_back(i);
        list2.push_back(i + 5);
    }

    // 测试splice整个列表
    list1.splice(list1.end(), list2);
    EXPECT_EQ(list1.size(), 10);
    EXPECT_TRUE(list2.empty());

    int expected = 0;
    for (auto it = list1.begin(); it != list1.end(); ++it)
    {
        EXPECT_EQ(*it, expected++);
    }

    // 测试splice部分元素
    list2.clear();
    for (int i = 0; i < 5; i++)
    {
        list2.push_back(i + 10);
    }

    auto it = list1.begin();
    std::advance(it, 5);
    list1.splice(it, list2, list2.begin(), list2.end());
    EXPECT_EQ(list1.size(), 15);
    EXPECT_TRUE(list2.empty());
}

// erase和remove测试
TEST_F(NFShmListTest, EraseAndRemoveTests)
{
    NFShmList<int, 20> list;

    // 准备数据
    for (int i = 0; i < 10; i++)
    {
        list.push_back(i);
    }

    // 测试erase单个元素
    auto it = list.begin();
    std::advance(it, 5);
    it = list.erase(it);
    EXPECT_EQ(*it, 6);
    EXPECT_EQ(list.size(), 9);

    // 测试erase范围
    it = list.begin();
    auto it2 = it;
    std::advance(it2, 3);
    it = list.erase(it, it2);
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(list.size(), 6);

    // 测试remove
    list.remove(5);
    EXPECT_EQ(list.size(), 6);

    // 测试remove_if
    list.remove_if([](int x) { return x % 2 == 0; });
    EXPECT_EQ(list.size(), 3);
}

// unique测试
TEST_F(NFShmListTest, UniqueTests)
{
    NFShmList<int, 20> list;

    // 准备数据
    list.push_back(1);
    list.push_back(1);
    list.push_back(2);
    list.push_back(2);
    list.push_back(3);
    list.push_back(3);
    list.push_back(3);

    // 测试unique
    list.unique();
    EXPECT_EQ(list.size(), 3);

    // 测试带谓词的unique
    list.clear();
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);
    list.push_back(5);

    list.unique([](int a, int b) { return (b - a) <= 1; });
    EXPECT_EQ(list.size(), 3);
}

// merge测试
TEST_F(NFShmListTest, MergeTests)
{
    NFShmList<int, 20> list1;
    NFShmList<int, 20> list2;

    // 准备数据
    for (int i = 0; i < 5; i += 2)
    {
        list1.push_back(i);
    }

    for (int i = 1; i < 6; i += 2)
    {
        list2.push_back(i);
    }

    // 测试merge
    list1.merge(list2);
    EXPECT_EQ(list1.size(), 6);
    EXPECT_TRUE(list2.empty());

    int expected = 0;
    for (auto it = list1.begin(); it != list1.end(); ++it)
    {
        EXPECT_EQ(*it, expected++);
    }

    // 测试带比较函数的merge
    list1.clear();
    list2.clear();

    for (int i = 0; i < 5; i++)
    {
        list1.push_back(i);
        list2.push_back(i + 5);
    }

    list1.merge(list2, std::greater<int>());
    EXPECT_EQ(list1.size(), 10);
    EXPECT_TRUE(list2.empty());
}

// reverse测试
TEST_F(NFShmListTest, ReverseTests)
{
    NFShmList<int, 10> list;

    // 准备数据
    for (int i = 0; i < 5; i++)
    {
        list.push_back(i);
    }

    // 测试reverse
    list.reverse();
    int expected = 4;
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        EXPECT_EQ(*it, expected--);
    }

    // 测试空列表reverse
    list.clear();
    EXPECT_NO_THROW(list.reverse());

    // 测试单元素列表reverse
    list.push_back(1);
    list.reverse();
    EXPECT_EQ(list.front(), 1);
}

// insert测试
TEST_F(NFShmListTest, InsertTests)
{
    NFShmList<int, 20> list;

    // 测试在开始位置插入
    list.insert(list.begin(), 1);
    EXPECT_EQ(list.front(), 1);

    // 测试在结束位置插入
    list.insert(list.end(), 2);
    EXPECT_EQ(list.back(), 2);

    // 测试在中间位置插入
    auto it = list.begin();
    ++it;
    list.insert(it, 3);
    EXPECT_EQ(list.size(), 3);

    // 测试插入多个元素
    it = list.begin();
    list.insert(it, 3, 4);
    EXPECT_EQ(list.size(), 6);

    // 测试插入范围
    std::vector<int> vec = {5, 6, 7};
    it = list.begin();
    list.insert(it, vec.begin(), vec.end());
    EXPECT_EQ(list.size(), 9);

    // 测试插入移动元素
    std::string str = "test";
    NFShmList<std::string, 10> strList;
    strList.insert(strList.begin(), std::move(str));
    EXPECT_EQ(strList.front(), "test");
}

// 组合操作测试
TEST_F(NFShmListTest, CombinedOperations)
{
    NFShmList<int, 30> list;

    // 准备数据
    for (int i = 0; i < 10; i++)
    {
        list.push_back(i);
    }

    // 测试组合操作
    // 1. 先reverse
    list.reverse();

    // 2. 再insert
    auto it = list.begin();
    std::advance(it, 5);
    list.insert(it, 3, 100);

    // 3. 然后remove
    list.remove(100);

    // 4. 最后unique
    list.unique();

    EXPECT_EQ(list.size(), 10);

    // 验证结果
    int expected = 9;
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        EXPECT_EQ(*it, expected--);
    }
}


