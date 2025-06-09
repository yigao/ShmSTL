// -------------------------------------------------------------------------
//    @FileName         :    NFShmQueue.h
//    @Author           :    gaoyi
//    @Date             :    24-9-6
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmQueue
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"
#include "NFShmList.h"

template<class Tp, int MAX_SIZE>
class NFShmQueue;

template<class Tp, int MAX_SIZE>
inline bool operator==(const NFShmQueue<Tp, MAX_SIZE> &, const NFShmQueue<Tp, MAX_SIZE> &);

template<class Tp, int MAX_SIZE>
inline bool operator<(const NFShmQueue<Tp, MAX_SIZE> &, const NFShmQueue<Tp, MAX_SIZE> &);

template<class Tp, int MAX_SIZE>
class NFShmQueue
{
    template<class Tp1, int MAX_SIZE1>
    friend bool operator==(const NFShmQueue<Tp1, MAX_SIZE1> &, const NFShmQueue<Tp1, MAX_SIZE1> &);

    template<class Tp1, int MAX_SIZE1>
    friend bool operator<(const NFShmQueue<Tp1, MAX_SIZE1> &, const NFShmQueue<Tp1, MAX_SIZE1> &);

public:
    typedef typename NFShmList<Tp, MAX_SIZE>::value_type value_type;
    typedef typename NFShmList<Tp, MAX_SIZE>::size_type size_type;
    typedef NFShmList<Tp, MAX_SIZE> container_type;

    typedef typename NFShmList<Tp, MAX_SIZE>::reference reference;
    typedef typename NFShmList<Tp, MAX_SIZE>::const_reference const_reference;

protected:
    NFShmList<Tp, MAX_SIZE> m_queue;

public:
    NFShmQueue()
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

    explicit NFShmQueue(const NFShmQueue &queue) : m_queue(queue)
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
    size_type max_size() const { return m_queue.max_size(); }
    bool full() const { return m_queue.full(); }
    reference front() { return m_queue.front(); }
    const_reference front() const { return m_queue.front(); }
    reference back() { return m_queue.back(); }
    const_reference back() const { return m_queue.back(); }
    void push(const value_type &__x) { m_queue.push_back(__x); }
    void pop() { m_queue.pop_front(); }
};

template<class Tp, int MAX_SIZE>
bool operator==(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return __x.c == __y.c;
}

template<class Tp, int MAX_SIZE>
bool operator<(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return __x.c < __y.c;
}

template<class Tp, int MAX_SIZE>
bool operator!=(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return !(__x == __y);
}

template<class Tp, int MAX_SIZE>
bool operator>(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return __y < __x;
}

template<class Tp, int MAX_SIZE>
bool operator<=(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return !(__y < __x);
}

template<class Tp, int MAX_SIZE>
bool operator>=(const NFShmQueue<Tp, MAX_SIZE> &__x, const NFShmQueue<Tp, MAX_SIZE> &__y)
{
    return !(__x < __y);
}
