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
#include "NFComm/NFShmStl/NFShmVector.h"

class TestA
{
public:
    TestA()
    {
        m_a = std::rand() % 10;
    }

    ~TestA()
    {
        m_a = -1;
    }

    int m_a;
};

class TestB
{
public:
    TestB()
    {
        m_a = "gaoyi";
    }

    ~TestB()
    {
    }

    std::string m_a;
};

TEST(NFShmVectorTest, AlignedStorege)
{
    NFShmVector<TestA, 2> vecTestA;
    vecTestA.emplace_back();
    NFShmVector<TestB, 2> vecTestB;
    vecTestB.emplace_back();
}

// 测试默认构造函数
TEST(NFShmVectorTest, DefaultConstructor)
{
    NFShmVector<int, 5> vec;
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 5);
    //测试越界访问元素
    EXPECT_EQ(&vec[0], &vec.m_staticError);

    NFShmVector<int, 5> vec2(2);
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(vec2.capacity(), 5);
    EXPECT_EQ(vec2[0], 0);
    EXPECT_EQ(vec2[1], 0);
    //测试越界访问元素
    EXPECT_EQ(&vec2[3], &vec2.m_staticError);

    NFShmVector<std::string, 5> vec3(5);
    EXPECT_EQ(vec3.size(), 5);
    EXPECT_EQ(vec3.capacity(), 5);
    EXPECT_EQ(vec3[0], "");
    EXPECT_EQ(vec3[1], "");
    EXPECT_EQ(vec3[2], "");
    EXPECT_EQ(vec3[3], "");
    EXPECT_EQ(vec3[4], "");
    //测试越界访问元素
    EXPECT_EQ(&vec3[5], &vec3.m_staticError);

    //越界测试
    NFShmVector<std::string, 2> vec4(5);
    EXPECT_EQ(vec4.size(), 2);
    EXPECT_EQ(vec4.capacity(), 2);
    EXPECT_EQ(vec4[0], "");
    EXPECT_EQ(vec4[1], "");
    //测试越界访问元素
    EXPECT_EQ(&vec4[3], &vec4.m_staticError);
}

TEST(NFShmVectorTest, ConstructorNValue)
{
    NFShmVector<std::string, 5> vec(2, "hi");
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec[0], "hi");
    EXPECT_EQ(vec[1], "hi");
    //测试越界访问元素
    EXPECT_EQ(&vec[2], &vec.m_staticError);

    NFShmVector<int, 5> vec2(2, 3);
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(vec2.capacity(), 5);
    EXPECT_EQ(vec2[0], 3);
    EXPECT_EQ(vec2[1], 3);
    //测试越界访问元素
    EXPECT_EQ(&vec2[2], &vec2.m_staticError);

    //越界测试
    NFShmVector<int, 2> vec3(3, 3);
    EXPECT_EQ(vec3.size(), 2);
    EXPECT_EQ(vec3.capacity(), 2);
    EXPECT_EQ(vec3[0], 3);
    EXPECT_EQ(vec3[1], 3);
    //测试越界访问元素
    EXPECT_EQ(&vec3[2], &vec3.m_staticError);
}

TEST(NFShmVectorTest, ConstructorList)
{
    NFShmVector<std::string, 5> vec{"why", "always", "me"};
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec[0], "why");
    EXPECT_EQ(vec[1], "always");
    EXPECT_EQ(vec[2], "me");
    //测试越界访问元素
    EXPECT_EQ(&vec[3], &vec.m_staticError);

    //越界测试
    vec.resize(6);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec.capacity(), 5);
    EXPECT_EQ(vec[0], "why");
    EXPECT_EQ(vec[1], "always");
    EXPECT_EQ(vec[2], "me");
    EXPECT_NE(&vec[3], &vec.m_staticError);
    EXPECT_EQ(vec[3], "");
    EXPECT_EQ(vec[5], "");

    //越界测试
    NFShmVector<std::string, 2> vec2{"why", "always", "me"};
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(vec2.capacity(), 2);
    EXPECT_EQ(vec2[0], "why");
    EXPECT_EQ(vec2[1], "always");
    //测试越界访问元素
    EXPECT_EQ(&vec2[2], &vec2.m_staticError);
}

TEST(NFShmVectorTest, ConstructorCopy)
{
    NFShmVector<std::string, 5> vec{"why", "always", "me"};
    NFShmVector<std::string, 5> vec2(vec.begin(), vec.end());
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(vec2.capacity(), 5);
    EXPECT_EQ(vec2[0], "why");
    EXPECT_EQ(vec2[1], "always");
    EXPECT_EQ(vec2[2], "me");
    EXPECT_EQ(vec2.front(), "why");
    EXPECT_EQ(vec2.back(), "me");

    std::list<std::string> list{"why", "always", "me"};
    NFShmVector<std::string, 3> vec3(list.begin(), list.end());
    EXPECT_EQ(vec3.size(), 3);
    EXPECT_EQ(vec3.capacity(), 3);
    EXPECT_EQ(vec3[0], "why");
    EXPECT_EQ(vec3[1], "always");
    EXPECT_EQ(vec3[2], "me");
    EXPECT_EQ(vec3.front(), "why");
    EXPECT_EQ(vec3.back(), "me");

    std::vector<std::string> vecStr{"why", "always", "me"};
    NFShmVector<std::string, 3> vec4(vecStr.begin(), vecStr.end());
    EXPECT_EQ(vec4.size(), 3);
    EXPECT_EQ(vec4.capacity(), 3);
    EXPECT_EQ(vec4[0], "why");
    EXPECT_EQ(vec4[1], "always");
    EXPECT_EQ(vec4[2], "me");
    EXPECT_EQ(vec4.front(), "why");
    EXPECT_EQ(vec4.back(), "me");

    NFShmVector<std::string, 3> vec5(vecStr);
    EXPECT_EQ(vec5.size(), 3);
    EXPECT_EQ(vec5.capacity(), 3);
    EXPECT_EQ(vec5[0], "why");
    EXPECT_EQ(vec5[1], "always");
    EXPECT_EQ(vec5[2], "me");
    EXPECT_EQ(vec5.front(), "why");
    EXPECT_EQ(vec5.back(), "me");

    NFShmVector<std::string, 3> vec6(vec);
    EXPECT_EQ(vec6.size(), 3);
    EXPECT_EQ(vec6.capacity(), 3);
    EXPECT_EQ(vec6[0], "why");
    EXPECT_EQ(vec6[1], "always");
    EXPECT_EQ(vec6[2], "me");
    EXPECT_EQ(vec6.front(), "why");
    EXPECT_EQ(vec6.back(), "me");

    NFShmVector<std::string, 2> vec7(vec);
    EXPECT_EQ(vec7.size(), 2);
    EXPECT_EQ(vec7.capacity(), 2);
    EXPECT_EQ(vec7[0], "why");
    EXPECT_EQ(vec7[1], "always");
    EXPECT_EQ(&vec7[2], &vec7.m_staticError);
    EXPECT_NE(vec7[2], "me");
    EXPECT_EQ(vec7.front(), "why");
    EXPECT_EQ(vec7.back(), "always");
}

// 测试带初始容量的构造函数
TEST(NFShmVectorTest, ConstructorWithCapacity)
{
    NFShmVector<int, 20> vec(10);
    EXPECT_EQ(vec.size(), 10);
    EXPECT_EQ(vec.capacity(), 20);
}

// 测试添加元素
TEST(NFShmVectorTest, PushBack)
{
    NFShmVector<int, 5> vec;
    vec.push_back(1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec.front(), 1);
    EXPECT_EQ(vec.back(), 1);
}

// 测试添加多个元素，触发扩容
TEST(NFShmVectorTest, PushBackMultiple)
{
    NFShmVector<int, 200> vec;
    for (int i = 0; i < 100; ++i)
    {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 100);
    EXPECT_EQ(vec.capacity(), 200);
}

// 测试删除最后一个元素
TEST(NFShmVectorTest, PopBack)
{
    NFShmVector<int, 10> vec;
    vec.push_back(1);
    vec.pop_back();
    EXPECT_EQ(vec.size(), 0);
}

TEST(NFShmVectorTest, Emplace)
{
    NFShmVector<int, 10> vec;
    vec.emplace_back();
    vec.emplace_back(1);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[1], 1);
    EXPECT_EQ(vec.front(), 0);
    EXPECT_EQ(vec.back(), 1);

    NFShmVector<std::string, 10> vec2;
    vec2.emplace_back();
    vec2.emplace_back("test");
    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(vec2[0], "");
    EXPECT_EQ(vec2[1], "test");
    EXPECT_EQ(vec2.front(), "");
    EXPECT_EQ(vec2.back(), "test");
    std::string tmp = "test";
    vec2.emplace_back(tmp);
    vec2.emplace(vec2.begin(), "test2");
    EXPECT_EQ(vec2[0], "test2");
    EXPECT_EQ(vec2[1], "");
    EXPECT_EQ(vec2[2], "test");
}

// 测试删除中间元素
TEST(NFShmVectorTest, Erase)
{
    NFShmVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.erase(vec.begin() + 1);
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 3);
    EXPECT_EQ(vec.front(), 1);
    EXPECT_EQ(vec.back(), 3);
    vec.erase(vec.begin(), vec.end());
    EXPECT_EQ(vec.size(), 0);

    NFShmVector<int, 10> vec2;
    vec2.push_back(1);
    vec2.push_back(2);
    vec2.push_back(3);
    vec2.push_back(4);
    vec2.push_back(5);
    vec2.push_back(6);
    vec2.push_back(7);
    vec2.push_back(8);
    vec2.push_back(9);
    vec2.push_back(10);
    for (auto it = vec2.begin(); it != vec2.end();)
    {
        if (*it % 2 == 0)
        {
            it = vec2.erase(it);
        }
        else
        {
            ++it;
        }
    }
    EXPECT_EQ(vec2.size(), 5);
    EXPECT_EQ(vec2[0], 1);
    EXPECT_EQ(vec2[1], 3);
    EXPECT_EQ(vec2[2], 5);
    EXPECT_EQ(vec2[3], 7);
    EXPECT_EQ(vec2[4], 9);
    EXPECT_EQ(vec2.front(), 1);
    EXPECT_EQ(vec2.back(), 9);
    //测试越界访问元素
    EXPECT_EQ(&vec2[5], &vec2.m_staticError);
}

//Insert
TEST(NFShmVectorTest, Insert)
{
    NFShmVector<int, 20> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.insert(vec.begin()+1, 4);
    auto iter = std::find_if(vec.begin(), vec.end(), [](int val) {return val == 3; });
    EXPECT_NE(iter, vec.end());
    vec.insert(iter, {4, 5, 6, 7});
    EXPECT_EQ(vec.size(), 8);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 4);
    EXPECT_EQ(vec[2], 2);
    EXPECT_EQ(vec[3], 4);
    EXPECT_EQ(vec[4], 5);
    EXPECT_EQ(vec[5], 6);
    EXPECT_EQ(vec[6], 7);
    EXPECT_EQ(vec[7], 3);
    //测试越界访问元素
    EXPECT_EQ(&vec[8], &vec.m_staticError);
    NFShmVector<int, 10> vec2;
    vec2.insert(vec2.begin(), {8, 9, 10});
    vec.insert(vec.end(), vec2.begin(), vec2.end());
    EXPECT_EQ(vec[8], 8);
    EXPECT_EQ(vec[9], 9);
    EXPECT_EQ(vec[10], 10);
    NFShmVector<int, 10> vec3;
    vec3.insert(vec3.end(), {11, 12, 13});
    vec.insert(vec.end(), vec3.begin(), vec3.end());
    EXPECT_EQ(vec[11], 11);
    EXPECT_EQ(vec[12], 12);
    EXPECT_EQ(vec[13], 13);
    vec.insert(vec.begin(), 4, 100);
    EXPECT_EQ(vec[0], 100);
    EXPECT_EQ(vec[1], 100);
    EXPECT_EQ(vec[2], 100);
}

// 测试正常访问元素
TEST(NFShmVectorTest, AccessElement)
{
    NFShmVector<int, 10> vec;
    vec.push_back(1);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec.at(0), 1);
    //
    EXPECT_EQ(&vec.at(1), &vec.m_staticError);
}

// 测试越界访问元素
TEST(NFShmVectorTest, AccessOutOfBounds)
{
    NFShmVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    EXPECT_EQ(&vec.at(2), &vec.m_staticError);
    EXPECT_EQ(&vec[3], &vec.m_staticError);
}

// 测试容量管理
TEST(NFShmVectorTest, CapacityManagement)
{
    NFShmVector<int, 10> vec;
    vec.resize(5);
    EXPECT_EQ(vec.capacity(), 10);
    vec.push_back(1);
    EXPECT_EQ(vec.size(), 6);
    EXPECT_EQ(vec.capacity(), 10);
}

// 测试迭代器操作
TEST(NFShmVectorTest, Iterator)
{
    NFShmVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);
    int rSum = 0;
    for (auto it = vec.rbegin(); it != vec.rend(); ++it)
    {
        rSum += *it;
    }
    EXPECT_EQ(rSum, 6);
    int cSum = 0;
    for (auto it = vec.cbegin(); it != vec.cend(); ++it)
    {
        cSum += *it;
    }
    EXPECT_EQ(cSum, 6);
    int crSum = 0;
    for (auto it = vec.crbegin(); it != vec.crend(); ++it)
    {
        crSum += *it;
    }
    EXPECT_EQ(crSum, 6);
}

// 测试空向量遍历
TEST(NFShmVectorTest, EmptyIterator)
{
    NFShmVector<int, 10> vec;
    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        sum += *it;
    }
    EXPECT_EQ(sum, 0);
}

TEST(NFShmVectorTest, Assign)
{
    NFShmVector<int, 10> vec;
    vec.insert(vec.begin(), {1, 2, 3});
    vec.assign({6, 7, 8, 9, 10});
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], 6);
    EXPECT_EQ(vec[1], 7);
    EXPECT_EQ(vec[2], 8);
    EXPECT_EQ(vec[3], 9);
    EXPECT_EQ(vec[4], 10);
    vec.assign({1, 2, 3});
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
    EXPECT_EQ(vec[2], 3);
}

TEST(NFShmVectorTest, Sort)
{
    NFShmVector<int, 10> vec;
    for (int i = 0; i < 10; ++i)
    {
        vec.push_back(i);
        EXPECT_EQ(vec[i], i);
    }
    vec.random_shuffle();
    EXPECT_EQ(vec.is_sorted(), false);
    vec.sort();
    EXPECT_EQ(vec.is_sorted(), true);
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(vec[i], i);
    }
    vec.sort(std::greater<int>());
    EXPECT_EQ(vec.is_sorted(), false);
    EXPECT_EQ(vec.is_sorted(std::greater<int>()), true);
    for (int i = 9, j = 0; i >= 0; --i,++j)
    {
        EXPECT_EQ(vec[j], i);
    }
    vec.remove(9);
    auto iter = std::find(vec.begin(), vec.end(), 9);
    EXPECT_EQ(iter, vec.end());
}

TEST(NFShmVectorTest, Binary)
{
    NFShmVector<int, 10> vec;
    int a = 0;
    for (int i = 0; i < 10; ++i)
    {
        int val = rand() % 100;
        auto iter = vec.binary_insert(val);
        EXPECT_NE(iter, vec.end());
        EXPECT_EQ(*iter, val);
        if (i == 5)
        {
            a = vec.back();
        }
    }
    EXPECT_EQ(vec.is_sorted(), true);
    auto iterA = vec.binary_search(a);
    EXPECT_NE(iterA, vec.end());
    EXPECT_EQ(*iterA, a);
    vec.binary_delete(a);
    auto iterD = vec.binary_search(a);
    EXPECT_EQ(iterD, vec.end());
}

