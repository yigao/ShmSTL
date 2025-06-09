// -------------------------------------------------------------------------
//    @FileName         :    NFShmStl.h
//    @Author           :    gaoyi
//    @Date             :    23-4-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmStl
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFStackTrace.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include <stdio.h>
#include <type_traits>

//#define TENCENT_USE_STL 1
#define USE_SHM_STL
#define SHM_CREATE_MODE EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()

//不适用bool, 如果内存没有初始化，很可能是一个随机值，导致初始化状态依然是true
enum NF_SHM_STL_INIT
{
    EN_NF_SHM_STL_INIT_NONE = 0,
    EN_NF_SHM_STL_INIT_OK = 1,
};

#ifdef TENCENT_USE_STL
#ifndef CHECK_EXPR
#define CHECK_EXPR(expr, ret, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
            CHECK_ERR(0, -1, format, ##__VA_ARGS__);\
            return ret;\
        }\
    }while(0)
#endif//CHECK_EXPR

#ifndef CHECK_EXPR_RE_VOID
#define CHECK_EXPR_RE_VOID(expr, format, ...)\
    do {\
        if (unlikely(!(expr)))\
        {\
            CHECK_ERR_RE_VOID(0, -1, format, ##__VA_ARGS__);\
        }\
    }while(0)
#endif//CHECK_EXPR_RE_VOID

#ifndef NF_ASSERT
#define NF_ASSERT(expr)\
    do {\
        if (unlikely(!(expr)))\
        {\
            CHECK_ERR_RE_VOID(0, -1, "assert error");\
        }\
    }while(0)
#endif//NF_ASSERT

#endif

#if NF_PLATFORM == NF_PLATFORM_WIN
namespace std
{
    //////////////////////////move.h//////////////////////////////////////
      template<typename _Tp>
    inline _Tp*
    __addressof(_Tp& __r)
    {
      return reinterpret_cast<_Tp*>
	(&const_cast<char&>(reinterpret_cast<const volatile char&>(__r)));
    }
    ///////////////////////////stl_construct.h////////////////////////////
    /**
     * Constructs an object in existing memory by invoking an allocated
     * object's constructor with an initializer.
     */
    template<typename _T1, typename... _Args>
    inline void
    _Construct(_T1* __p, _Args&&... __args)
    { ::new(static_cast<void*>(__p)) _T1(std::forward<_Args>(__args)...); }

    /**
     * Destroy the object pointed to by a pointer type.
     */
    template<typename _Tp>
    inline void
    _Destroy(_Tp* __pointer)
    { __pointer->~_Tp(); }

    template<bool>
    struct _Destroy_aux
    {
        template<typename _ForwardIterator>
        static void
        __destroy(_ForwardIterator __first, _ForwardIterator __last)
        {
            for (; __first != __last; ++__first)
                std::_Destroy(std::__addressof(*__first));
        }
    };

    template<>
    struct _Destroy_aux<true>
    {
        template<typename _ForwardIterator>
        static void
        __destroy(_ForwardIterator, _ForwardIterator) { }
    };

    /**
     * Destroy a range of objects.  If the value_type of the object has
     * a trivial destructor, the compiler should optimize all of this
     * away, otherwise the objects' destructors must be invoked.
     */
    template<typename _ForwardIterator>
    inline void
    _Destroy(_ForwardIterator __first, _ForwardIterator __last)
    {
        typedef typename iterator_traits<_ForwardIterator>::value_type
                _Value_type;
        std::_Destroy_aux<__has_trivial_destructor(_Value_type)>::
        __destroy(__first, __last);
    }

    /**
     * Destroy a range of objects using the supplied allocator.  For
     * nondefault allocators we do not optimize away invocation of
     * destroy() even if _Tp has a trivial destructor.
     */

    ///////////////////////////stl_function.h////////////////////////////
    template<typename _Arg, typename _Result>
    struct stl_unary_function
    {
        /// @c argument_type is the type of the argument
        typedef _Arg 	argument_type;

        /// @c result_type is the return type
        typedef _Result 	result_type;
    };

    template<typename _Tp>
    struct stl__Identity
            : public stl_unary_function<_Tp,_Tp>
    {
        _Tp&
        operator()(_Tp& __x) const
        { return __x; }

        const _Tp&
        operator()(const _Tp& __x) const
        { return __x; }
    };

    template<typename _Pair>
    struct _Select1st
            : public stl_unary_function<_Pair, typename _Pair::first_type>
    {
        typename _Pair::first_type&
        operator()(_Pair& __x) const
        { return __x.first; }

        const typename _Pair::first_type&
        operator()(const _Pair& __x) const
        { return __x.first; }

        template<typename _Pair2>
        typename _Pair2::first_type&
        operator()(_Pair2& __x) const
        { return __x.first; }

        template<typename _Pair2>
        const typename _Pair2::first_type&
        operator()(const _Pair2& __x) const
        { return __x.first; }
    };

    ///////////////////////cpp_type_traits.h////////////////////////////////////
    struct __true_type { };
    struct __false_type { };

    template<bool>
    struct __truth_type
    { typedef __false_type __type; };

    template<>
    struct __truth_type<true>
    { typedef __true_type __type; };

    // N.B. The conversions to bool are needed due to the issue
    // explained in c++/19404.
    template<class _Sp, class _Tp>
    struct __traitor
    {
        enum { __value = bool(_Sp::__value) || bool(_Tp::__value) };
        typedef typename __truth_type<__value>::__type __type;
    };

    // Compare for equality of types.
    template<typename, typename>
    struct __are_same
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<typename _Tp>
    struct __are_same<_Tp, _Tp>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    // Holds if the template-argument is a void type.
    template<typename _Tp>
    struct __is_void
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<>
    struct __is_void<void>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    //
    // Integer types
    //
    template<typename _Tp>
    struct __is_integer
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    // Thirteen specializations (yes there are eleven standard integer
    // types; <em>long long</em> and <em>unsigned long long</em> are
    // supported as extensions)
    template<>
    struct __is_integer<bool>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<signed char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<unsigned char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

# ifdef _GLIBCXX_USE_WCHAR_T
    template<>
    struct __is_integer<wchar_t>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };
# endif

    template<>
    struct __is_integer<char16_t>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<char32_t>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<short>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<unsigned short>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<int>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<unsigned int>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<long>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<unsigned long>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<long long>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_integer<unsigned long long>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    //
    // Floating point types
    //
    template<typename _Tp>
    struct __is_floating
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    // three specializations (float, double and 'long double')
    template<>
    struct __is_floating<float>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_floating<double>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_floating<long double>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    //
    // Pointer types
    //
    template<typename _Tp>
    struct __is_pointer
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<typename _Tp>
    struct __is_pointer<_Tp*>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    //
    // Normal iterator type
    //
    template<typename _Tp>
    struct __is_normal_iterator
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    //
    // An arithmetic type is an integer type or a floating point type
    //
    template<typename _Tp>
    struct __is_arithmetic
            : public __traitor<__is_integer<_Tp>, __is_floating<_Tp> >
    { };

    //
    // A fundamental type is `void' or and arithmetic type
    //
    template<typename _Tp>
    struct __is_fundamental
            : public __traitor<__is_void<_Tp>, __is_arithmetic<_Tp> >
    { };

    //
    // A scalar type is an arithmetic type or a pointer type
    //
    template<typename _Tp>
    struct __is_scalar
            : public __traitor<__is_arithmetic<_Tp>, __is_pointer<_Tp> >
    { };

    //
    // For use in std::copy and std::find overloads for streambuf iterators.
    //
    template<typename _Tp>
    struct __is_char
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<>
    struct __is_char<char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

#ifdef _GLIBCXX_USE_WCHAR_T
    template<>
    struct __is_char<wchar_t>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };
#endif

    template<typename _Tp>
    struct __is_byte
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<>
    struct __is_byte<char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_byte<signed char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<>
    struct __is_byte<unsigned char>
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    //
    // Move iterator type
    //
    template<typename _Tp>
    struct __is_move_iterator
    {
        enum { __value = 0 };
        typedef __false_type __type;
    };

    template<typename _Iterator>
    class move_iterator;

    template<typename _Iterator>
    struct __is_move_iterator< move_iterator<_Iterator> >
    {
        enum { __value = 1 };
        typedef __true_type __type;
    };

    template<typename ForwardIterator, typename Size>
    void __uninitialized_default_n(ForwardIterator first, Size n)
    {
        typedef typename iterator_traits<ForwardIterator>::value_type ValueType;
        if (!std::stl_is_trivially_default_constructible<ValueType>::value)
        {
            for (size_t i = 0; i < n; i++)
            {
                new(first + i) ValueType(); // 标准placement构造
            }
        }
        else
        {
            std::memset(first, 0, n * sizeof(ValueType));
        }
    }

} // namespace std

#else

#define stl__Identity _Identity
#if __cplusplus >= 201402L
#else

#endif

#endif

namespace std
{
    // 前置声明
    template <typename T>
    struct stl_is_trivially_default_constructible;

    // 主模板
    template <typename T>
    struct stl_is_trivially_default_constructible
    {
    private:
        template <typename U>
        static typename std::integral_constant<bool,
                                              std::is_trivial<U>::value &&
                                              std::is_default_constructible<U>::value
        >::type test(int);

        template <typename>
        static std::false_type test(...);

    public:
        static const bool value = decltype(test<T>(0))::value;
    };
}