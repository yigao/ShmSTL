// -------------------------------------------------------------------------
//    @FileName         :    NFShmPriorityQueue.h
//    @Author           :    gaoyi
//    @Date             :    24-9-9
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmPriorityQueue
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"
#include "NFShmVector.h"


template<class Tp, int MAX_SIZE, class Compare = std::less<Tp> >
class NFShmPriorityQueue
{
public:
    typedef typename NFShmVector<Tp, MAX_SIZE>::value_type value_type;
    typedef typename NFShmVector<Tp, MAX_SIZE>::size_type size_type;
    typedef NFShmVector<Tp, MAX_SIZE> container_type;

    typedef typename NFShmVector<Tp, MAX_SIZE>::reference reference;
    typedef typename NFShmVector<Tp, MAX_SIZE>::const_reference const_reference;

protected:
    NFShmVector<Tp, MAX_SIZE> m_queue;
    Compare m_comp;
    int8_t m_init;

public:
    NFShmPriorityQueue() : m_queue()
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

    int CreateInit()
    {
        m_init = EN_NF_SHM_STL_INIT_OK;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    explicit NFShmPriorityQueue(const Compare &__x) : m_queue(), m_comp(__x)
    {
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    NFShmPriorityQueue(const Compare &x, const NFShmVector<Tp, MAX_SIZE> &s)
        : m_queue(s), m_comp(x)
    {
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    template<class _InputIterator>
    NFShmPriorityQueue(_InputIterator __first, _InputIterator __last)
        : m_queue(__first, __last)
    {
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    template<class _InputIterator>
    NFShmPriorityQueue(_InputIterator __first,
                       _InputIterator __last, const Compare &__x)
        : m_queue(__first, __last), m_comp(__x)
    {
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    template<class _InputIterator>
    NFShmPriorityQueue(_InputIterator __first, _InputIterator __last,
                       const Compare &__x, const NFShmVector<Tp, MAX_SIZE> &__s)
        : m_queue(__s), m_comp(__x)
    {
        m_queue.insert(m_queue.end(), __first, __last);
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    NFShmPriorityQueue(const value_type *__first, const value_type *__last)
        : m_queue(__first, __last)
    {
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    NFShmPriorityQueue(const value_type *__first, const value_type *__last,
                       const Compare &__x)
        : m_queue(__first, __last), m_comp(__x)
    {
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    NFShmPriorityQueue(const value_type *__first, const value_type *__last,
                       const Compare &__x, const NFShmVector<Tp, MAX_SIZE> &__c)
        : m_queue(__c), m_comp(__x)
    {
        m_queue.insert(m_queue.end(), __first, __last);
        make_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    bool empty() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "__c not init", TRACE_STACK());
        return m_queue.empty();
    }

    size_type size() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "__c not init", TRACE_STACK());
        return m_queue.size();
    }

    const_reference top() const { return m_queue.front(); }

    void push(const value_type &__x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "__c not init", TRACE_STACK());
        m_queue.push_back(__x);
        push_heap(m_queue.begin(), m_queue.end(), m_comp);
    }

    void pop()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "__c not init", TRACE_STACK());
        pop_heap(m_queue.begin(), m_queue.end(), m_comp);
        m_queue.pop_back();
    }
};
