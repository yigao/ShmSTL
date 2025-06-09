// -------------------------------------------------------------------------
//    @FileName         :    NFShmBitSet.h
//    @Author           :    gaoyi
//    @Date             :    24-9-9
//    @Email            :    445267987@qq.com
//    @Module           :    NFShmBitSet
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmStl.h"

#include <stddef.h>     // for size_t
#include <string.h>     // for memset
#include <string>
#include <stdexcept>    // for invalid_argument, out_of_range, overflow_error
#include <iostream>

/**
*
size() 返回 std::bitset 的长度
count() 返回 std::bitset 中值为 1 的位的数量
any() 返回 std::bitset 中是否存在值为 1 的位
none() 返回 std::bitset 中是否所有位都是 0
all() 返回 std::bitset 中是否所有位都是 1
test(pos) 返回 std::bitset 中位于 pos 位置的值
set(pos) 将 std::bitset 中位于 pos 位置的值设为 1
reset(pos) 将 std::bitset 中位于 pos 位置的值设为 0
flip(pos) 将 std::bitset 中位于 pos 位置的值取反
to_ulong() 返回 std::bitset 转换成的无符号整数值
to_ullong() 返回 std::bitset 转换成的无符号长整数值
 */
#define __BITS_PER_WORD (CHAR_BIT*sizeof(unsigned long))
#define __BITSET_WORDS(__n) \
 ((__n) < 1 ? 1 : ((__n) + __BITS_PER_WORD - 1)/__BITS_PER_WORD)

// structure to aid in counting bits

template<bool __dummy>
struct _Bit_count
{
    static unsigned char _S_bit_count[256];
};

// Mapping from 8 bit unsigned integers to the index of the first one
// bit:
template<bool __dummy>
struct _First_one
{
    static unsigned char _S_first_one[256];
};

//
// Base class: general case.
//

template<size_t _Nw>
class NFShmBaseBitSet
{
public:
    typedef unsigned long _WordT;

    _WordT _M_w[_Nw]; // 0 is the least significant word.
    int8_t m_init;

    NFShmBaseBitSet()
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
        _M_do_reset();
        m_init = EN_NF_SHM_STL_INIT_OK;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    NFShmBaseBitSet(unsigned long __val)
    {
        _M_do_reset();
        _M_w[0] = __val;
        m_init = EN_NF_SHM_STL_INIT_OK;
    }

    static size_t _S_whichword(size_t __pos)
    {
        return __pos / __BITS_PER_WORD;
    }

    static size_t _S_whichbyte(size_t __pos)
    {
        return (__pos % __BITS_PER_WORD) / CHAR_BIT;
    }

    static size_t _S_whichbit(size_t __pos)
    {
        return __pos % __BITS_PER_WORD;
    }

    static _WordT _S_maskbit(size_t __pos)
    {
        return (static_cast<_WordT>(1)) << _S_whichbit(__pos);
    }

    _WordT &_M_getword(size_t __pos)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, _M_w[_S_whichword(__pos)], "not init, TRACE_STACK:%s", TRACE_STACK());
        return _M_w[_S_whichword(__pos)];
    }

    _WordT _M_getword(size_t __pos) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, _M_w[_S_whichword(__pos)], "not init, TRACE_STACK:%s", TRACE_STACK());
        return _M_w[_S_whichword(__pos)];
    }

    _WordT &_M_hiword()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, _M_w[_Nw - 1], "not init, TRACE_STACK:%s", TRACE_STACK());
        return _M_w[_Nw - 1];
    }

    _WordT _M_hiword() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, _M_w[_Nw - 1], "not init, TRACE_STACK:%s", TRACE_STACK());
        return _M_w[_Nw - 1];
    }

    void _M_do_and(const NFShmBaseBitSet<_Nw> &__x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            _M_w[__i] &= __x._M_w[__i];
        }
    }

    void _M_do_or(const NFShmBaseBitSet<_Nw> &__x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            _M_w[__i] |= __x._M_w[__i];
        }
    }

    void _M_do_xor(const NFShmBaseBitSet<_Nw> &__x)
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            _M_w[__i] ^= __x._M_w[__i];
        }
    }

    void _M_do_left_shift(size_t __shift);

    void _M_do_right_shift(size_t __shift);

    void _M_do_flip()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            _M_w[__i] = ~_M_w[__i];
        }
    }

    void _M_do_set()
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            _M_w[__i] = ~static_cast<_WordT>(0);
        }
    }

    void _M_do_reset() { memset(_M_w, 0, _Nw * sizeof(_WordT)); }

    bool _M_is_equal(const NFShmBaseBitSet<_Nw> &__x) const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; ++__i)
        {
            if (_M_w[__i] != __x._M_w[__i])
                return false;
        }
        return true;
    }

    bool _M_is_any() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, false, "not init, TRACE_STACK:%s", TRACE_STACK());
        for (size_t __i = 0; __i < _Nw; __i++)
        {
            if (_M_w[__i] != static_cast<_WordT>(0))
                return true;
        }
        return false;
    }

    size_t _M_do_count() const
    {
        CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, 0, "not init, TRACE_STACK:%s", TRACE_STACK());
        size_t __result = 0;
        const unsigned char *__byte_ptr = (const unsigned char *) _M_w;
        const unsigned char *__end_ptr = (const unsigned char *) (_M_w + _Nw);

        while (__byte_ptr < __end_ptr)
        {
            __result += _Bit_count<true>::_S_bit_count[*__byte_ptr];
            __byte_ptr++;
        }
        return __result;
    }

    unsigned long _M_do_to_ulong() const;

    // find first "on" bit
    size_t _M_do_find_first(size_t __not_found) const;

    // find the next "on" bit that follows "prev"
    size_t _M_do_find_next(size_t __prev, size_t __not_found) const;
};

template<size_t _Nw>
void NFShmBaseBitSet<_Nw>::_M_do_left_shift(size_t __shift)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
    if (__shift != 0)
    {
        const size_t __wshift = __shift / __BITS_PER_WORD;
        const size_t __offset = __shift % __BITS_PER_WORD;

        if (__offset == 0)
            for (size_t __n = _Nw - 1; __n >= __wshift; --__n)
                _M_w[__n] = _M_w[__n - __wshift];

        else
        {
            const size_t __sub_offset = __BITS_PER_WORD - __offset;
            for (size_t __n = _Nw - 1; __n > __wshift; --__n)
                _M_w[__n] = (_M_w[__n - __wshift] << __offset) |
                            (_M_w[__n - __wshift - 1] >> __sub_offset);
            _M_w[__wshift] = _M_w[0] << __offset;
        }

        std::fill(_M_w + 0, _M_w + __wshift, static_cast<_WordT>(0));
    }
}

template<size_t _Nw>
void NFShmBaseBitSet<_Nw>::_M_do_right_shift(size_t __shift)
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, , "not init, TRACE_STACK:%s", TRACE_STACK());
    if (__shift != 0)
    {
        const size_t __wshift = __shift / __BITS_PER_WORD;
        const size_t __offset = __shift % __BITS_PER_WORD;
        const size_t __limit = _Nw - __wshift - 1;

        if (__offset == 0)
            for (size_t __n = 0; __n <= __limit; ++__n)
                _M_w[__n] = _M_w[__n + __wshift];

        else
        {
            const size_t __sub_offset = __BITS_PER_WORD - __offset;
            for (size_t __n = 0; __n < __limit; ++__n)
                _M_w[__n] = (_M_w[__n + __wshift] >> __offset) |
                            (_M_w[__n + __wshift + 1] << __sub_offset);
            _M_w[__limit] = _M_w[_Nw - 1] >> __offset;
        }

        std::fill(_M_w + __limit + 1, _M_w + _Nw, static_cast<_WordT>(0));
    }
}

template<size_t _Nw>
unsigned long NFShmBaseBitSet<_Nw>::_M_do_to_ulong() const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, _M_w[0], "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_t __i = 1; __i < _Nw; ++__i)
    {
        if (_M_w[__i])
        {
            LOG_ERR(0, -1, "overflow error, TRACE_STACK:%s", TRACE_STACK());
            return _M_w[0];
        }
    }

    return _M_w[0];
}

template<size_t _Nw>
size_t NFShmBaseBitSet<_Nw>::_M_do_find_first(size_t __not_found) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, __not_found, "not init, TRACE_STACK:%s", TRACE_STACK());
    for (size_t __i = 0; __i < _Nw; __i++)
    {
        _WordT __thisword = _M_w[__i];
        if (__thisword != static_cast<_WordT>(0))
        {
            // find byte within word
            for (size_t __j = 0; __j < sizeof(_WordT); __j++)
            {
                unsigned char __this_byte
                        = static_cast<unsigned char>(__thisword & (~(unsigned char) 0));
                if (__this_byte)
                    return __i * __BITS_PER_WORD + __j * CHAR_BIT +
                           _First_one<true>::_S_first_one[__this_byte];

                __thisword >>= CHAR_BIT;
            }
        }
    }
    // not found, so return an indication of failure.
    return __not_found;
}

template<size_t _Nw>
size_t NFShmBaseBitSet<_Nw>::_M_do_find_next(size_t __prev, size_t __not_found) const
{
    CHECK_EXPR(m_init == EN_NF_SHM_STL_INIT_OK, __not_found, "not init, TRACE_STACK:%s", TRACE_STACK());
    // make bound inclusive
    ++__prev;

    // check out of bounds
    if (__prev >= _Nw * __BITS_PER_WORD)
        return __not_found;

    // search first word
    size_t __i = _S_whichword(__prev);
    _WordT __thisword = _M_w[__i];

    // mask off bits below bound
    __thisword &= (~static_cast<_WordT>(0)) << _S_whichbit(__prev);

    if (__thisword != static_cast<_WordT>(0))
    {
        // find byte within word
        // get first byte into place
        __thisword >>= _S_whichbyte(__prev) * CHAR_BIT;
        for (size_t __j = _S_whichbyte(__prev); __j < sizeof(_WordT); __j++)
        {
            unsigned char __this_byte
                    = static_cast<unsigned char>(__thisword & (~(unsigned char) 0));
            if (__this_byte)
                return __i * __BITS_PER_WORD + __j * CHAR_BIT +
                       _First_one<true>::_S_first_one[__this_byte];

            __thisword >>= CHAR_BIT;
        }
    }

    // check subsequent words
    __i++;
    for (; __i < _Nw; __i++)
    {
        _WordT __thisword = _M_w[__i];
        if (__thisword != static_cast<_WordT>(0))
        {
            // find byte within word
            for (size_t __j = 0; __j < sizeof(_WordT); __j++)
            {
                unsigned char __this_byte
                        = static_cast<unsigned char>(__thisword & (~(unsigned char) 0));
                if (__this_byte)
                    return __i * __BITS_PER_WORD + __j * CHAR_BIT +
                           _First_one<true>::_S_first_one[__this_byte];

                __thisword >>= CHAR_BIT;
            }
        }
    }

    // not found, so return an indication of failure.
    return __not_found;
} // end _M_do_find_next


// ------------------------------------------------------------

//
// Base class: specialization for a single word.
//

template<>
class NFShmBaseBitSet<1>
{
public:
    typedef unsigned long _WordT;
    _WordT _M_w;

    NFShmBaseBitSet(void) : _M_w(0)
    {
    }

    NFShmBaseBitSet(unsigned long __val) : _M_w(__val)
    {
    }

    static size_t _S_whichword(size_t __pos)
    {
        return __pos / __BITS_PER_WORD;
    }

    static size_t _S_whichbyte(size_t __pos)
    {
        return (__pos % __BITS_PER_WORD) / CHAR_BIT;
    }

    static size_t _S_whichbit(size_t __pos)
    {
        return __pos % __BITS_PER_WORD;
    }

    static _WordT _S_maskbit(size_t __pos)
    {
        return (static_cast<_WordT>(1)) << _S_whichbit(__pos);
    }

    _WordT &_M_getword(size_t) { return _M_w; }
    _WordT _M_getword(size_t) const { return _M_w; }

    _WordT &_M_hiword() { return _M_w; }
    _WordT _M_hiword() const { return _M_w; }

    void _M_do_and(const NFShmBaseBitSet<1> &__x) { _M_w &= __x._M_w; }
    void _M_do_or(const NFShmBaseBitSet<1> &__x) { _M_w |= __x._M_w; }
    void _M_do_xor(const NFShmBaseBitSet<1> &__x) { _M_w ^= __x._M_w; }
    void _M_do_left_shift(size_t __shift) { _M_w <<= __shift; }
    void _M_do_right_shift(size_t __shift) { _M_w >>= __shift; }
    void _M_do_flip() { _M_w = ~_M_w; }
    void _M_do_set() { _M_w = ~static_cast<_WordT>(0); }
    void _M_do_reset() { _M_w = 0; }

    bool _M_is_equal(const NFShmBaseBitSet<1> &__x) const
    {
        return _M_w == __x._M_w;
    }

    bool _M_is_any() const
    {
        return _M_w != 0;
    }

    size_t _M_do_count() const
    {
        size_t __result = 0;
        const unsigned char *__byte_ptr = (const unsigned char *) &_M_w;
        const unsigned char *__end_ptr
                = ((const unsigned char *) &_M_w) + sizeof(_M_w);
        while (__byte_ptr < __end_ptr)
        {
            __result += _Bit_count<true>::_S_bit_count[*__byte_ptr];
            __byte_ptr++;
        }
        return __result;
    }

    unsigned long _M_do_to_ulong() const { return _M_w; }

    size_t _M_do_find_first(size_t __not_found) const
    {
        _WordT __thisword = _M_w;

        if (__thisword != static_cast<_WordT>(0))
        {
            // find byte within word
            for (size_t __j = 0; __j < sizeof(_WordT); __j++)
            {
                unsigned char __this_byte
                        = static_cast<unsigned char>(__thisword & (~(unsigned char) 0));
                if (__this_byte)
                    return __j * CHAR_BIT + _First_one<true>::_S_first_one[__this_byte];

                __thisword >>= CHAR_BIT;
            }
        }
        // not found, so return a value that indicates failure.
        return __not_found;
    }

    // find the next "on" bit that follows "prev"
    size_t _M_do_find_next(size_t __prev, size_t __not_found) const
    {
        // make bound inclusive
        ++__prev;

        // check out of bounds
        if (__prev >= __BITS_PER_WORD)
            return __not_found;

        // search first (and only) word
        _WordT __thisword = _M_w;

        // mask off bits below bound
        __thisword &= (~static_cast<_WordT>(0)) << _S_whichbit(__prev);

        if (__thisword != static_cast<_WordT>(0))
        {
            // find byte within word
            // get first byte into place
            __thisword >>= _S_whichbyte(__prev) * CHAR_BIT;
            for (size_t __j = _S_whichbyte(__prev); __j < sizeof(_WordT); __j++)
            {
                unsigned char __this_byte
                        = static_cast<unsigned char>(__thisword & (~(unsigned char) 0));
                if (__this_byte)
                    return __j * CHAR_BIT + _First_one<true>::_S_first_one[__this_byte];

                __thisword >>= CHAR_BIT;
            }
        }

        // not found, so return a value that indicates failure.
        return __not_found;
    } // end _M_do_find_next
};

//
// Definitions of non-inline functions from the single-word version of
//  _Base_bitset.
//

template<size_t _Extrabits>
struct NFSanitize
{
    static void _M_do_sanitize(unsigned long &__val)
    {
        __val &= ~((~static_cast<unsigned long>(0)) << _Extrabits);
    }
};

template<>
struct NFSanitize<0>
{
    static void _M_do_sanitize(unsigned long)
    {
    }
};

template<size_t _Nb>
class NFShmBitSet : private NFShmBaseBitSet<__BITSET_WORDS(_Nb)>
{
private:
    typedef NFShmBaseBitSet<__BITSET_WORDS(_Nb)> _Base;
    typedef unsigned long _WordT;

private:
    void _M_do_sanitize()
    {
        NFSanitize<_Nb % __BITS_PER_WORD>::_M_do_sanitize(this->_M_hiword());
    }

public:
    // bit reference:
    class reference;
    friend class reference;

    class reference
    {
        friend class NFShmBitSet;

        _WordT *_M_wp;
        size_t _M_bpos;

        // left undefined
        reference();

    public:
        reference(NFShmBitSet &__b, size_t __pos)
        {
            _M_wp = &__b._M_getword(__pos);
            _M_bpos = _Base::_S_whichbit(__pos);
        }

        ~reference()
        {
        }

        // for b[i] = __x;
        reference &operator=(bool __x)
        {
            if (__x)
                *_M_wp |= _Base::_S_maskbit(_M_bpos);
            else
                *_M_wp &= ~_Base::_S_maskbit(_M_bpos);

            return *this;
        }

        // for b[i] = b[__j];
        reference &operator=(const reference &__j)
        {
            if ((*(__j._M_wp) & _Base::_S_maskbit(__j._M_bpos)))
                *_M_wp |= _Base::_S_maskbit(_M_bpos);
            else
                *_M_wp &= ~_Base::_S_maskbit(_M_bpos);

            return *this;
        }

        // flips the bit
        bool operator~() const
        {
            return (*(_M_wp) & _Base::_S_maskbit(_M_bpos)) == 0;
        }

        // for __x = b[i];
        operator bool() const
        {
            return (*(_M_wp) & _Base::_S_maskbit(_M_bpos)) != 0;
        }

        // for b[i].flip();
        reference &flip()
        {
            *_M_wp ^= _Base::_S_maskbit(_M_bpos);
            return *this;
        }
    };

    NFShmBitSet()
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
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    NFShmBitSet(unsigned long __val) : NFShmBaseBitSet<__BITSET_WORDS(_Nb)>(__val)
    {
        _M_do_sanitize();
    }

    explicit NFShmBitSet(const std::string &__s, size_t __pos = 0) : _Base()
    {
        CHECK_EXPR(__pos <= __s.size(), , "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        _M_copy_from_string(__s, __pos,
                             std::string::npos);
    }

    NFShmBitSet(const std::string &__s, size_t __pos, size_t __n = std::string::npos) : _Base()
    {
        CHECK_EXPR(__pos <= __s.size(), , "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        _M_copy_from_string(__s, __pos, __n);
    }

    // 23.3.5.2 bitset operations:
    NFShmBitSet<_Nb> &operator&=(const NFShmBitSet<_Nb> &__rhs)
    {
        this->_M_do_and(__rhs);
        return *this;
    }

    NFShmBitSet<_Nb> &operator|=(const NFShmBitSet<_Nb> &__rhs)
    {
        this->_M_do_or(__rhs);
        return *this;
    }

    NFShmBitSet<_Nb> &operator^=(const NFShmBitSet<_Nb> &__rhs)
    {
        this->_M_do_xor(__rhs);
        return *this;
    }

    NFShmBitSet<_Nb> &operator<<=(size_t __pos)
    {
        this->_M_do_left_shift(__pos);
        this->_M_do_sanitize();
        return *this;
    }

    NFShmBitSet<_Nb> &operator>>=(size_t __pos)
    {
        this->_M_do_right_shift(__pos);
        this->_M_do_sanitize();
        return *this;
    }

    //
    // Extension:
    // Versions of single-bit set, reset, flip, test with no range checking.
    //

    NFShmBitSet<_Nb> &_Unchecked_set(size_t __pos)
    {
        this->_M_getword(__pos) |= _Base::_S_maskbit(__pos);
        return *this;
    }

    NFShmBitSet<_Nb> &_Unchecked_set(size_t __pos, int __val)
    {
        if (__val)
            this->_M_getword(__pos) |= _Base::_S_maskbit(__pos);
        else
            this->_M_getword(__pos) &= ~_Base::_S_maskbit(__pos);

        return *this;
    }

    NFShmBitSet<_Nb> &_Unchecked_reset(size_t __pos)
    {
        this->_M_getword(__pos) &= ~_Base::_S_maskbit(__pos);
        return *this;
    }

    NFShmBitSet<_Nb> &_Unchecked_flip(size_t __pos)
    {
        this->_M_getword(__pos) ^= _Base::_S_maskbit(__pos);
        return *this;
    }

    bool _Unchecked_test(size_t __pos) const
    {
        return (this->_M_getword(__pos) & _Base::_S_maskbit(__pos))
               != static_cast<_WordT>(0);
    }

    // Set, reset, and flip.

    NFShmBitSet<_Nb> &set()
    {
        this->_M_do_set();
        this->_M_do_sanitize();
        return *this;
    }

    NFShmBitSet<_Nb> &set(size_t __pos)
    {
        CHECK_EXPR(__pos < _Nb, *this, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());

        return _Unchecked_set(__pos);
    }

    NFShmBitSet<_Nb> &set(size_t __pos, int __val)
    {
        CHECK_EXPR(__pos < _Nb, *this, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());

        return _Unchecked_set(__pos, __val);
    }

    NFShmBitSet<_Nb> &reset()
    {
        this->_M_do_reset();
        return *this;
    }

    NFShmBitSet<_Nb> &reset(size_t __pos)
    {
        CHECK_EXPR(__pos < _Nb, *this, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());

        return _Unchecked_reset(__pos);
    }

    NFShmBitSet<_Nb> &flip()
    {
        this->_M_do_flip();
        this->_M_do_sanitize();
        return *this;
    }

    NFShmBitSet<_Nb> &flip(size_t __pos)
    {
        CHECK_EXPR(__pos < _Nb, *this, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());

        return _Unchecked_flip(__pos);
    }

    NFShmBitSet<_Nb> operator~() const
    {
        return NFShmBitSet<_Nb>(*this).flip();
    }

    // element access:
    //for b[i];
    reference operator[](size_t __pos)
    {
        CHECK_EXPR(__pos < _Nb,  reference(*this, 0), "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        return reference(*this, __pos);
    }

    bool operator[](size_t __pos) const
    {
        CHECK_EXPR(__pos < _Nb, false, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());
        return _Unchecked_test(__pos);
    }

    unsigned long to_ulong() const { return this->_M_do_to_ulong(); }

    std::string to_string() const
    {
        std::string __result;
        _M_copy_to_string(__result);
        return __result;
    }

    void _M_copy_from_string(const std::string &__s, size_t, size_t);

    void _M_copy_to_string(std::string &) const;

    size_t count() const { return this->_M_do_count(); }

    size_t size() const { return _Nb; }

    bool operator==(const NFShmBitSet<_Nb> &__rhs) const
    {
        return this->_M_is_equal(__rhs);
    }

    bool operator!=(const NFShmBitSet<_Nb> &__rhs) const
    {
        return !this->_M_is_equal(__rhs);
    }

    bool test(size_t __pos) const
    {
        CHECK_EXPR(__pos < _Nb, false, "out of range bitset, TRACE_STACK:%s", TRACE_STACK());

        return _Unchecked_test(__pos);
    }

    bool any() const { return this->_M_is_any(); }
    bool none() const { return !this->_M_is_any(); }
    bool all() const { return count() == size(); }

    NFShmBitSet<_Nb> operator<<(size_t __pos) const
    {
        return NFShmBitSet<_Nb>(*this) <<= __pos;
    }

    NFShmBitSet<_Nb> operator>>(size_t __pos) const
    {
        return NFShmBitSet<_Nb>(*this) >>= __pos;
    }

    //
    // EXTENSIONS: bit-find operations.  These operations are
    // experimental, and are subject to change or removal in future
    // versions.
    //

    // find the index of the first "on" bit
    size_t _Find_first() const
    {
        return this->_M_do_find_first(_Nb);
    }

    // find the index of the next "on" bit after prev
    size_t _Find_next(size_t __prev) const
    {
        return this->_M_do_find_next(__prev, _Nb);
    }
};


template<size_t _Nb>
void NFShmBitSet<_Nb>::_M_copy_from_string(const std::string &__s, size_t __pos, size_t __n)
{
    reset();
    const size_t __nbits = min(_Nb, min(__n, __s.size() - __pos));
    for (size_t __i = 0; __i < __nbits; ++__i)
    {
        switch (__s[__pos + __nbits - __i - 1])
        {
            case '0':
                break;
            case '1':
                set(__i);
                break;
            default:
                LOG_ERR(0, -1, "invalid_argument bitset, TRACE_STACK:%s", TRACE_STACK());
        }
    }
}

template<size_t _Nb>
void NFShmBitSet<_Nb>::_M_copy_to_string(std::string &__s) const
{
    __s.assign(_Nb, '0');

    for (size_t __i = 0; __i < _Nb; ++__i)
        if (_Unchecked_test(__i))
            __s[_Nb - 1 - __i] = '1';
}

template<size_t _Nb>
inline NFShmBitSet<_Nb> operator&(const NFShmBitSet<_Nb> &__x, const NFShmBitSet<_Nb> &__y)
{
    NFShmBitSet<_Nb> __result(__x);
    __result &= __y;
    return __result;
}


template<size_t _Nb>
inline NFShmBitSet<_Nb> operator|(const NFShmBitSet<_Nb> &__x, const NFShmBitSet<_Nb> &__y)
{
    NFShmBitSet<_Nb> __result(__x);
    __result |= __y;
    return __result;
}

template<size_t _Nb>
inline NFShmBitSet<_Nb> operator^(const NFShmBitSet<_Nb> &__x, const NFShmBitSet<_Nb> &__y)
{
    NFShmBitSet<_Nb> __result(__x);
    __result ^= __y;
    return __result;
}

template<class _CharT, class _Traits, size_t _Nb>
std::basic_istream<_CharT, _Traits> &operator>>(std::basic_istream<_CharT, _Traits> &__is, NFShmBitSet<_Nb> &__x)
{
    std::basic_string<_CharT, _Traits> __tmp;
    __tmp.reserve(_Nb);

    // Skip whitespace
    typename std::basic_istream<_CharT, _Traits>::sentry __sentry(__is);
    if (__sentry)
    {
        std::basic_streambuf<_CharT, _Traits> *__buf = __is.rdbuf();
        for (size_t __i = 0; __i < _Nb; ++__i)
        {
            static typename _Traits::int_type __eof = _Traits::eof();

            typename _Traits::int_type __c1 = __buf->sbumpc();
            if (_Traits::eq_int_type(__c1, __eof))
            {
                __is.setstate(ios_base::eofbit);
                break;
            }
            else
            {
                char __c2 = _Traits::to_char_type(__c1);
                char __c = __is.narrow(__c2, '*');

                if (__c == '0' || __c == '1')
                    __tmp.push_back(__c);
                else if (_Traits::eq_int_type(__buf->sputbackc(__c2), __eof))
                {
                    __is.setstate(ios_base::failbit);
                    break;
                }
            }
        }

        if (__tmp.empty())
            __is.setstate(ios_base::failbit);
        else
            __x._M_copy_from_string(__tmp, static_cast<size_t>(0), _Nb);
    }

    return __is;
}

template<class _CharT, class _Traits, size_t _Nb>
std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &__os, const NFShmBitSet<_Nb> &__x)
{
    std::basic_string<_CharT, _Traits> __tmp;
    __x._M_copy_to_string(__tmp);
    return __os << __tmp;
}

template<size_t _Nb>
std::istream &operator>>(std::istream &__is, NFShmBitSet<_Nb> &__x)
{
    string __tmp;
    __tmp.reserve(_Nb);

    if (__is.flags() & ios::skipws)
    {
        char __c;
        do
            __is.get(__c);
        while (__is && isspace(__c));
        if (__is)
            __is.putback(__c);
    }

    for (size_t __i = 0; __i < _Nb; ++__i)
    {
        char __c;
        __is.get(__c);

        if (!__is)
            break;
        else if (__c != '0' && __c != '1')
        {
            __is.putback(__c);
            break;
        }
        else
            __tmp.push_back(__c);
    }

    if (__tmp.empty())
        __is.clear(__is.rdstate() | ios::failbit);
    else
        __x._M_copy_from_string(__tmp, static_cast<size_t>(0), _Nb);

    return __is;
}

template<size_t _Nb>
std::ostream &operator<<(std::ostream &__os, const NFShmBitSet<_Nb> &__x)
{
    string __tmp;
    __x._M_copy_to_string(__tmp);
    return __os << __tmp;
}

// ------------------------------------------------------------
// Lookup tables for find and count operations.

template<bool __dummy>
unsigned char _Bit_count<__dummy>::_S_bit_count[] = {
    0, /*   0 */ 1, /*   1 */ 1, /*   2 */ 2, /*   3 */ 1, /*   4 */
    2, /*   5 */ 2, /*   6 */ 3, /*   7 */ 1, /*   8 */ 2, /*   9 */
    2, /*  10 */ 3, /*  11 */ 2, /*  12 */ 3, /*  13 */ 3, /*  14 */
    4, /*  15 */ 1, /*  16 */ 2, /*  17 */ 2, /*  18 */ 3, /*  19 */
    2, /*  20 */ 3, /*  21 */ 3, /*  22 */ 4, /*  23 */ 2, /*  24 */
    3, /*  25 */ 3, /*  26 */ 4, /*  27 */ 3, /*  28 */ 4, /*  29 */
    4, /*  30 */ 5, /*  31 */ 1, /*  32 */ 2, /*  33 */ 2, /*  34 */
    3, /*  35 */ 2, /*  36 */ 3, /*  37 */ 3, /*  38 */ 4, /*  39 */
    2, /*  40 */ 3, /*  41 */ 3, /*  42 */ 4, /*  43 */ 3, /*  44 */
    4, /*  45 */ 4, /*  46 */ 5, /*  47 */ 2, /*  48 */ 3, /*  49 */
    3, /*  50 */ 4, /*  51 */ 3, /*  52 */ 4, /*  53 */ 4, /*  54 */
    5, /*  55 */ 3, /*  56 */ 4, /*  57 */ 4, /*  58 */ 5, /*  59 */
    4, /*  60 */ 5, /*  61 */ 5, /*  62 */ 6, /*  63 */ 1, /*  64 */
    2, /*  65 */ 2, /*  66 */ 3, /*  67 */ 2, /*  68 */ 3, /*  69 */
    3, /*  70 */ 4, /*  71 */ 2, /*  72 */ 3, /*  73 */ 3, /*  74 */
    4, /*  75 */ 3, /*  76 */ 4, /*  77 */ 4, /*  78 */ 5, /*  79 */
    2, /*  80 */ 3, /*  81 */ 3, /*  82 */ 4, /*  83 */ 3, /*  84 */
    4, /*  85 */ 4, /*  86 */ 5, /*  87 */ 3, /*  88 */ 4, /*  89 */
    4, /*  90 */ 5, /*  91 */ 4, /*  92 */ 5, /*  93 */ 5, /*  94 */
    6, /*  95 */ 2, /*  96 */ 3, /*  97 */ 3, /*  98 */ 4, /*  99 */
    3, /* 100 */ 4, /* 101 */ 4, /* 102 */ 5, /* 103 */ 3, /* 104 */
    4, /* 105 */ 4, /* 106 */ 5, /* 107 */ 4, /* 108 */ 5, /* 109 */
    5, /* 110 */ 6, /* 111 */ 3, /* 112 */ 4, /* 113 */ 4, /* 114 */
    5, /* 115 */ 4, /* 116 */ 5, /* 117 */ 5, /* 118 */ 6, /* 119 */
    4, /* 120 */ 5, /* 121 */ 5, /* 122 */ 6, /* 123 */ 5, /* 124 */
    6, /* 125 */ 6, /* 126 */ 7, /* 127 */ 1, /* 128 */ 2, /* 129 */
    2, /* 130 */ 3, /* 131 */ 2, /* 132 */ 3, /* 133 */ 3, /* 134 */
    4, /* 135 */ 2, /* 136 */ 3, /* 137 */ 3, /* 138 */ 4, /* 139 */
    3, /* 140 */ 4, /* 141 */ 4, /* 142 */ 5, /* 143 */ 2, /* 144 */
    3, /* 145 */ 3, /* 146 */ 4, /* 147 */ 3, /* 148 */ 4, /* 149 */
    4, /* 150 */ 5, /* 151 */ 3, /* 152 */ 4, /* 153 */ 4, /* 154 */
    5, /* 155 */ 4, /* 156 */ 5, /* 157 */ 5, /* 158 */ 6, /* 159 */
    2, /* 160 */ 3, /* 161 */ 3, /* 162 */ 4, /* 163 */ 3, /* 164 */
    4, /* 165 */ 4, /* 166 */ 5, /* 167 */ 3, /* 168 */ 4, /* 169 */
    4, /* 170 */ 5, /* 171 */ 4, /* 172 */ 5, /* 173 */ 5, /* 174 */
    6, /* 175 */ 3, /* 176 */ 4, /* 177 */ 4, /* 178 */ 5, /* 179 */
    4, /* 180 */ 5, /* 181 */ 5, /* 182 */ 6, /* 183 */ 4, /* 184 */
    5, /* 185 */ 5, /* 186 */ 6, /* 187 */ 5, /* 188 */ 6, /* 189 */
    6, /* 190 */ 7, /* 191 */ 2, /* 192 */ 3, /* 193 */ 3, /* 194 */
    4, /* 195 */ 3, /* 196 */ 4, /* 197 */ 4, /* 198 */ 5, /* 199 */
    3, /* 200 */ 4, /* 201 */ 4, /* 202 */ 5, /* 203 */ 4, /* 204 */
    5, /* 205 */ 5, /* 206 */ 6, /* 207 */ 3, /* 208 */ 4, /* 209 */
    4, /* 210 */ 5, /* 211 */ 4, /* 212 */ 5, /* 213 */ 5, /* 214 */
    6, /* 215 */ 4, /* 216 */ 5, /* 217 */ 5, /* 218 */ 6, /* 219 */
    5, /* 220 */ 6, /* 221 */ 6, /* 222 */ 7, /* 223 */ 3, /* 224 */
    4, /* 225 */ 4, /* 226 */ 5, /* 227 */ 4, /* 228 */ 5, /* 229 */
    5, /* 230 */ 6, /* 231 */ 4, /* 232 */ 5, /* 233 */ 5, /* 234 */
    6, /* 235 */ 5, /* 236 */ 6, /* 237 */ 6, /* 238 */ 7, /* 239 */
    4, /* 240 */ 5, /* 241 */ 5, /* 242 */ 6, /* 243 */ 5, /* 244 */
    6, /* 245 */ 6, /* 246 */ 7, /* 247 */ 5, /* 248 */ 6, /* 249 */
    6, /* 250 */ 7, /* 251 */ 6, /* 252 */ 7, /* 253 */ 7, /* 254 */
    8 /* 255 */
}; // end _Bit_count

template<bool __dummy>
unsigned char _First_one<__dummy>::_S_first_one[] = {
    0, /*   0 */ 0, /*   1 */ 1, /*   2 */ 0, /*   3 */ 2, /*   4 */
    0, /*   5 */ 1, /*   6 */ 0, /*   7 */ 3, /*   8 */ 0, /*   9 */
    1, /*  10 */ 0, /*  11 */ 2, /*  12 */ 0, /*  13 */ 1, /*  14 */
    0, /*  15 */ 4, /*  16 */ 0, /*  17 */ 1, /*  18 */ 0, /*  19 */
    2, /*  20 */ 0, /*  21 */ 1, /*  22 */ 0, /*  23 */ 3, /*  24 */
    0, /*  25 */ 1, /*  26 */ 0, /*  27 */ 2, /*  28 */ 0, /*  29 */
    1, /*  30 */ 0, /*  31 */ 5, /*  32 */ 0, /*  33 */ 1, /*  34 */
    0, /*  35 */ 2, /*  36 */ 0, /*  37 */ 1, /*  38 */ 0, /*  39 */
    3, /*  40 */ 0, /*  41 */ 1, /*  42 */ 0, /*  43 */ 2, /*  44 */
    0, /*  45 */ 1, /*  46 */ 0, /*  47 */ 4, /*  48 */ 0, /*  49 */
    1, /*  50 */ 0, /*  51 */ 2, /*  52 */ 0, /*  53 */ 1, /*  54 */
    0, /*  55 */ 3, /*  56 */ 0, /*  57 */ 1, /*  58 */ 0, /*  59 */
    2, /*  60 */ 0, /*  61 */ 1, /*  62 */ 0, /*  63 */ 6, /*  64 */
    0, /*  65 */ 1, /*  66 */ 0, /*  67 */ 2, /*  68 */ 0, /*  69 */
    1, /*  70 */ 0, /*  71 */ 3, /*  72 */ 0, /*  73 */ 1, /*  74 */
    0, /*  75 */ 2, /*  76 */ 0, /*  77 */ 1, /*  78 */ 0, /*  79 */
    4, /*  80 */ 0, /*  81 */ 1, /*  82 */ 0, /*  83 */ 2, /*  84 */
    0, /*  85 */ 1, /*  86 */ 0, /*  87 */ 3, /*  88 */ 0, /*  89 */
    1, /*  90 */ 0, /*  91 */ 2, /*  92 */ 0, /*  93 */ 1, /*  94 */
    0, /*  95 */ 5, /*  96 */ 0, /*  97 */ 1, /*  98 */ 0, /*  99 */
    2, /* 100 */ 0, /* 101 */ 1, /* 102 */ 0, /* 103 */ 3, /* 104 */
    0, /* 105 */ 1, /* 106 */ 0, /* 107 */ 2, /* 108 */ 0, /* 109 */
    1, /* 110 */ 0, /* 111 */ 4, /* 112 */ 0, /* 113 */ 1, /* 114 */
    0, /* 115 */ 2, /* 116 */ 0, /* 117 */ 1, /* 118 */ 0, /* 119 */
    3, /* 120 */ 0, /* 121 */ 1, /* 122 */ 0, /* 123 */ 2, /* 124 */
    0, /* 125 */ 1, /* 126 */ 0, /* 127 */ 7, /* 128 */ 0, /* 129 */
    1, /* 130 */ 0, /* 131 */ 2, /* 132 */ 0, /* 133 */ 1, /* 134 */
    0, /* 135 */ 3, /* 136 */ 0, /* 137 */ 1, /* 138 */ 0, /* 139 */
    2, /* 140 */ 0, /* 141 */ 1, /* 142 */ 0, /* 143 */ 4, /* 144 */
    0, /* 145 */ 1, /* 146 */ 0, /* 147 */ 2, /* 148 */ 0, /* 149 */
    1, /* 150 */ 0, /* 151 */ 3, /* 152 */ 0, /* 153 */ 1, /* 154 */
    0, /* 155 */ 2, /* 156 */ 0, /* 157 */ 1, /* 158 */ 0, /* 159 */
    5, /* 160 */ 0, /* 161 */ 1, /* 162 */ 0, /* 163 */ 2, /* 164 */
    0, /* 165 */ 1, /* 166 */ 0, /* 167 */ 3, /* 168 */ 0, /* 169 */
    1, /* 170 */ 0, /* 171 */ 2, /* 172 */ 0, /* 173 */ 1, /* 174 */
    0, /* 175 */ 4, /* 176 */ 0, /* 177 */ 1, /* 178 */ 0, /* 179 */
    2, /* 180 */ 0, /* 181 */ 1, /* 182 */ 0, /* 183 */ 3, /* 184 */
    0, /* 185 */ 1, /* 186 */ 0, /* 187 */ 2, /* 188 */ 0, /* 189 */
    1, /* 190 */ 0, /* 191 */ 6, /* 192 */ 0, /* 193 */ 1, /* 194 */
    0, /* 195 */ 2, /* 196 */ 0, /* 197 */ 1, /* 198 */ 0, /* 199 */
    3, /* 200 */ 0, /* 201 */ 1, /* 202 */ 0, /* 203 */ 2, /* 204 */
    0, /* 205 */ 1, /* 206 */ 0, /* 207 */ 4, /* 208 */ 0, /* 209 */
    1, /* 210 */ 0, /* 211 */ 2, /* 212 */ 0, /* 213 */ 1, /* 214 */
    0, /* 215 */ 3, /* 216 */ 0, /* 217 */ 1, /* 218 */ 0, /* 219 */
    2, /* 220 */ 0, /* 221 */ 1, /* 222 */ 0, /* 223 */ 5, /* 224 */
    0, /* 225 */ 1, /* 226 */ 0, /* 227 */ 2, /* 228 */ 0, /* 229 */
    1, /* 230 */ 0, /* 231 */ 3, /* 232 */ 0, /* 233 */ 1, /* 234 */
    0, /* 235 */ 2, /* 236 */ 0, /* 237 */ 1, /* 238 */ 0, /* 239 */
    4, /* 240 */ 0, /* 241 */ 1, /* 242 */ 0, /* 243 */ 2, /* 244 */
    0, /* 245 */ 1, /* 246 */ 0, /* 247 */ 3, /* 248 */ 0, /* 249 */
    1, /* 250 */ 0, /* 251 */ 2, /* 252 */ 0, /* 253 */ 1, /* 254 */
    0, /* 255 */
}; // end _First_one
