// -------------------------------------------------------------------------
//    @FileName         :    NFShmStack.h
//    @Author           :    gaoyi
//    @Date             :    24-9-9
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmStack
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"
#include "NFShmList.h"

template<class Tp, int MAX_SIZE>
class NFShmStack;

template<class Tp, int MAX_SIZE>
inline bool operator==(const NFShmStack<Tp, MAX_SIZE> &, const NFShmStack<Tp, MAX_SIZE> &);

template<class Tp, int MAX_SIZE>
inline bool operator<(const NFShmStack<Tp, MAX_SIZE> &, const NFShmStack<Tp, MAX_SIZE> &);

template<class Tp, int MAX_SIZE>
class NFShmStack
{
    template<class Tp1, int MAX_SIZE1>
    friend bool operator==(const NFShmStack<Tp1, MAX_SIZE1> &, const NFShmStack<Tp1, MAX_SIZE1> &);

    template<class Tp1, int MAX_SIZE1>
    friend bool operator<(const NFShmStack<Tp1, MAX_SIZE1> &, const NFShmStack<Tp1, MAX_SIZE1> &);

public:
    typedef typename NFShmList<Tp, MAX_SIZE>::value_type value_type;
    typedef typename NFShmList<Tp, MAX_SIZE>::size_type size_type;
    typedef NFShmList<Tp, MAX_SIZE> container_type;

    typedef typename NFShmList<Tp, MAX_SIZE>::reference reference;
    typedef typename NFShmList<Tp, MAX_SIZE>::const_reference const_reference;

protected:
    NFShmList<Tp, MAX_SIZE> m_queue;

public:
    NFShmStack()
    {
        if (SHM_CREATE_MODE)
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    explicit NFShmStack(const NFShmStack &queue) : m_queue(queue)
    {
    }

    int CreateInit()
    {
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    bool empty() const { return m_queue.empty(); }
    size_type size() const { return m_queue.size(); }
    reference top() { return m_queue.back(); }
    const_reference top() const { return m_queue.back(); }
    void push(const value_type &__x) { m_queue.push_back(__x); }
    void pop() { m_queue.pop_back(); }
};

template<class Tp, int MAX_SIZE>
bool operator==(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return __x.c == __y.c;
}

template<class Tp, int MAX_SIZE>
bool operator<(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return __x.c < __y.c;
}

template<class Tp, int MAX_SIZE>
bool operator!=(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return !(__x == __y);
}

template<class Tp, int MAX_SIZE>
bool operator>(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return __y < __x;
}

template<class Tp, int MAX_SIZE>
bool operator<=(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return !(__y < __x);
}

template<class Tp, int MAX_SIZE>
bool operator>=(const NFShmStack<Tp, MAX_SIZE> &__x, const NFShmStack<Tp, MAX_SIZE> &__y)
{
    return !(__x < __y);
}