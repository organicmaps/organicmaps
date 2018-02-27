
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2016 Daniel James
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_IMPLEMENTATION_HPP
#define BOOST_UNORDERED_DETAIL_IMPLEMENTATION_HPP

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/assert.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/detail/select_type.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/limits.hpp>
#include <boost/move/move.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/swap.hpp>
#include <boost/throw_exception.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/add_lvalue_reference.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/type_traits/is_nothrow_move_assignable.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/unordered/detail/fwd.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/utility/enable_if.hpp>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)
#include <tuple>
#endif

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>
#endif

////////////////////////////////////////////////////////////////////////////////
// Configuration
//
// Unless documented elsewhere these configuration macros should be considered
// an implementation detail, I'll try not to break them, but you never know.

// BOOST_UNORDERED_EMPLACE_LIMIT = The maximum number of parameters in emplace
// (not including things like hints). Don't set it to a lower value, as that
// might break something.

#if !defined BOOST_UNORDERED_EMPLACE_LIMIT
#define BOOST_UNORDERED_EMPLACE_LIMIT 11
#endif

// BOOST_UNORDERED_INTEROPERABLE_NODES - Use the same node type for
// containers with unique and equivalent keys.
//
// 0 = Use different nodes
// 1 = Use ungrouped nodes everywhere
//
// Might add an extra value to use grouped nodes everywhere later.

#if !defined(BOOST_UNORDERED_INTEROPERABLE_NODES)
#define BOOST_UNORDERED_INTEROPERABLE_NODES 0
#endif

// BOOST_UNORDERED_USE_ALLOCATOR_TRAITS - Pick which version of
// allocator_traits to use.
//
// 0 = Own partial implementation
// 1 = std::allocator_traits
// 2 = boost::container::allocator_traits

#if !defined(BOOST_UNORDERED_USE_ALLOCATOR_TRAITS)
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 1
#elif defined(BOOST_MSVC)
#if BOOST_MSVC < 1400
// Use container's allocator_traits for older versions of Visual
// C++ as I don't test with them.
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 2
#endif
#endif
#endif

#if !defined(BOOST_UNORDERED_USE_ALLOCATOR_TRAITS)
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 0
#endif

namespace boost {
namespace unordered {
namespace iterator_detail {
template <typename Node> struct iterator;
template <typename Node> struct c_iterator;
template <typename Node, typename Policy> struct l_iterator;
template <typename Node, typename Policy> struct cl_iterator;
}
}
}

namespace boost {
namespace unordered {
namespace detail {

template <typename Types> struct table;
template <typename NodePointer> struct bucket;
struct ptr_bucket;
template <typename Types> struct table_impl;
template <typename Types> struct grouped_table_impl;

template <typename A, typename T> struct unique_node;
template <typename T> struct ptr_node;
template <typename Types> struct table_impl;

template <typename A, typename T> struct grouped_node;
template <typename T> struct grouped_ptr_node;
template <typename Types> struct grouped_table_impl;
template <typename N> struct node_algo;
template <typename N> struct grouped_node_algo;

static const float minimum_max_load_factor = 1e-3f;
static const std::size_t default_bucket_count = 11;
struct move_tag
{
};
struct empty_emplace
{
};

namespace func {
template <class T> inline void ignore_unused_variable_warning(T const&) {}
}

////////////////////////////////////////////////////////////////////////////
// iterator SFINAE

template <typename I>
struct is_forward
    : boost::is_convertible<typename boost::iterator_traversal<I>::type,
          boost::forward_traversal_tag>
{
};

template <typename I, typename ReturnType>
struct enable_if_forward
    : boost::enable_if_c<boost::unordered::detail::is_forward<I>::value,
          ReturnType>
{
};

template <typename I, typename ReturnType>
struct disable_if_forward
    : boost::disable_if_c<boost::unordered::detail::is_forward<I>::value,
          ReturnType>
{
};

////////////////////////////////////////////////////////////////////////////
// primes

// clang-format off
#define BOOST_UNORDERED_PRIMES \
    (17ul)(29ul)(37ul)(53ul)(67ul)(79ul) \
    (97ul)(131ul)(193ul)(257ul)(389ul)(521ul)(769ul) \
    (1031ul)(1543ul)(2053ul)(3079ul)(6151ul)(12289ul)(24593ul) \
    (49157ul)(98317ul)(196613ul)(393241ul)(786433ul) \
    (1572869ul)(3145739ul)(6291469ul)(12582917ul)(25165843ul) \
    (50331653ul)(100663319ul)(201326611ul)(402653189ul)(805306457ul) \
    (1610612741ul)(3221225473ul)(4294967291ul)
// clang-format on

template <class T> struct prime_list_template
{
    static std::size_t const value[];

#if !defined(SUNPRO_CC)
    static std::ptrdiff_t const length;
#else
    static std::ptrdiff_t const length =
        BOOST_PP_SEQ_SIZE(BOOST_UNORDERED_PRIMES);
#endif
};

template <class T>
std::size_t const prime_list_template<T>::value[] = {
    BOOST_PP_SEQ_ENUM(BOOST_UNORDERED_PRIMES)};

#if !defined(SUNPRO_CC)
template <class T>
std::ptrdiff_t const prime_list_template<T>::length = BOOST_PP_SEQ_SIZE(
    BOOST_UNORDERED_PRIMES);
#endif

#undef BOOST_UNORDERED_PRIMES

typedef prime_list_template<std::size_t> prime_list;

// no throw
inline std::size_t next_prime(std::size_t num)
{
    std::size_t const* const prime_list_begin = prime_list::value;
    std::size_t const* const prime_list_end =
        prime_list_begin + prime_list::length;
    std::size_t const* bound =
        std::lower_bound(prime_list_begin, prime_list_end, num);
    if (bound == prime_list_end)
        bound--;
    return *bound;
}

// no throw
inline std::size_t prev_prime(std::size_t num)
{
    std::size_t const* const prime_list_begin = prime_list::value;
    std::size_t const* const prime_list_end =
        prime_list_begin + prime_list::length;
    std::size_t const* bound =
        std::upper_bound(prime_list_begin, prime_list_end, num);
    if (bound != prime_list_begin)
        bound--;
    return *bound;
}

////////////////////////////////////////////////////////////////////////////
// insert_size/initial_size

template <class I>
inline std::size_t insert_size(I i, I j,
    typename boost::unordered::detail::enable_if_forward<I, void*>::type = 0)
{
    return static_cast<std::size_t>(std::distance(i, j));
}

template <class I>
inline std::size_t insert_size(I, I,
    typename boost::unordered::detail::disable_if_forward<I, void*>::type = 0)
{
    return 1;
}

template <class I>
inline std::size_t initial_size(I i, I j,
    std::size_t num_buckets = boost::unordered::detail::default_bucket_count)
{
    // TODO: Why +1?
    return (std::max)(
        boost::unordered::detail::insert_size(i, j) + 1, num_buckets);
}

////////////////////////////////////////////////////////////////////////////
// compressed

template <typename T, int Index> struct compressed_base : private T
{
    compressed_base(T const& x) : T(x) {}
    compressed_base(T& x, move_tag) : T(boost::move(x)) {}

    T& get() { return *this; }
    T const& get() const { return *this; }
};

template <typename T, int Index> struct uncompressed_base
{
    uncompressed_base(T const& x) : value_(x) {}
    uncompressed_base(T& x, move_tag) : value_(boost::move(x)) {}

    T& get() { return value_; }
    T const& get() const { return value_; }
  private:
    T value_;
};

template <typename T, int Index>
struct generate_base
    : boost::detail::if_true<boost::is_empty<T>::value>::BOOST_NESTED_TEMPLATE
          then<boost::unordered::detail::compressed_base<T, Index>,
              boost::unordered::detail::uncompressed_base<T, Index> >
{
};

template <typename T1, typename T2>
struct compressed
    : private boost::unordered::detail::generate_base<T1, 1>::type,
      private boost::unordered::detail::generate_base<T2, 2>::type
{
    typedef typename generate_base<T1, 1>::type base1;
    typedef typename generate_base<T2, 2>::type base2;

    typedef T1 first_type;
    typedef T2 second_type;

    first_type& first() { return static_cast<base1*>(this)->get(); }

    first_type const& first() const
    {
        return static_cast<base1 const*>(this)->get();
    }

    second_type& second() { return static_cast<base2*>(this)->get(); }

    second_type const& second() const
    {
        return static_cast<base2 const*>(this)->get();
    }

    template <typename First, typename Second>
    compressed(First const& x1, Second const& x2) : base1(x1), base2(x2)
    {
    }

    compressed(compressed const& x) : base1(x.first()), base2(x.second()) {}

    compressed(compressed& x, move_tag m)
        : base1(x.first(), m), base2(x.second(), m)
    {
    }

    void assign(compressed const& x)
    {
        first() = x.first();
        second() = x.second();
    }

    void move_assign(compressed& x)
    {
        first() = boost::move(x.first());
        second() = boost::move(x.second());
    }

    void swap(compressed& x)
    {
        boost::swap(first(), x.first());
        boost::swap(second(), x.second());
    }

  private:
    // Prevent assignment just to make use of assign or
    // move_assign explicit.
    compressed& operator=(compressed const&);
};

////////////////////////////////////////////////////////////////////////////
// pair_traits
//
// Used to get the types from a pair without instantiating it.

template <typename Pair> struct pair_traits
{
    typedef typename Pair::first_type first_type;
    typedef typename Pair::second_type second_type;
};

template <typename T1, typename T2> struct pair_traits<std::pair<T1, T2> >
{
    typedef T1 first_type;
    typedef T2 second_type;
};

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4512) // assignment operator could not be generated.
#pragma warning(disable : 4345) // behavior change: an object of POD type
// constructed with an initializer of the form ()
// will be default-initialized.
#endif

////////////////////////////////////////////////////////////////////////////
// Bits and pieces for implementing traits

template <typename T> typename boost::add_lvalue_reference<T>::type make();
struct choice9
{
    typedef char (&type)[9];
};
struct choice8 : choice9
{
    typedef char (&type)[8];
};
struct choice7 : choice8
{
    typedef char (&type)[7];
};
struct choice6 : choice7
{
    typedef char (&type)[6];
};
struct choice5 : choice6
{
    typedef char (&type)[5];
};
struct choice4 : choice5
{
    typedef char (&type)[4];
};
struct choice3 : choice4
{
    typedef char (&type)[3];
};
struct choice2 : choice3
{
    typedef char (&type)[2];
};
struct choice1 : choice2
{
    typedef char (&type)[1];
};
choice1 choose();

typedef choice1::type yes_type;
typedef choice2::type no_type;

struct private_type
{
    private_type const& operator,(int) const;
};

template <typename T> no_type is_private_type(T const&);
yes_type is_private_type(private_type const&);

struct convert_from_anything
{
    template <typename T> convert_from_anything(T const&);
};

namespace func {
// This is a bit nasty, when constructing the individual members
// of a std::pair, need to cast away 'const'. For modern compilers,
// should be able to use std::piecewise_construct instead.
template <typename T> T* const_cast_pointer(T* x) { return x; }
template <typename T> T* const_cast_pointer(T const* x)
{
    return const_cast<T*>(x);
}
}

////////////////////////////////////////////////////////////////////////////
// emplace_args
//
// Either forwarding variadic arguments, or storing the arguments in
// emplace_args##n

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_EMPLACE_ARGS1(a0) a0
#define BOOST_UNORDERED_EMPLACE_ARGS2(a0, a1) a0, a1
#define BOOST_UNORDERED_EMPLACE_ARGS3(a0, a1, a2) a0, a1, a2

#define BOOST_UNORDERED_EMPLACE_TEMPLATE typename... Args
#define BOOST_UNORDERED_EMPLACE_ARGS BOOST_FWD_REF(Args)... args
#define BOOST_UNORDERED_EMPLACE_FORWARD boost::forward<Args>(args)...

#else

#define BOOST_UNORDERED_EMPLACE_ARGS1 create_emplace_args
#define BOOST_UNORDERED_EMPLACE_ARGS2 create_emplace_args
#define BOOST_UNORDERED_EMPLACE_ARGS3 create_emplace_args

#define BOOST_UNORDERED_EMPLACE_TEMPLATE typename Args
#define BOOST_UNORDERED_EMPLACE_ARGS Args const& args
#define BOOST_UNORDERED_EMPLACE_FORWARD args

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#define BOOST_UNORDERED_EARGS_MEMBER(z, n, _)                                  \
    typedef BOOST_FWD_REF(BOOST_PP_CAT(A, n)) BOOST_PP_CAT(Arg, n);            \
    BOOST_PP_CAT(Arg, n) BOOST_PP_CAT(a, n);

#else

#define BOOST_UNORDERED_EARGS_MEMBER(z, n, _)                                  \
    typedef typename boost::add_lvalue_reference<BOOST_PP_CAT(A, n)>::type     \
        BOOST_PP_CAT(Arg, n);                                                  \
    BOOST_PP_CAT(Arg, n) BOOST_PP_CAT(a, n);

#endif

template <typename A0> struct emplace_args1
{
    BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)

    explicit emplace_args1(Arg0 b0) : a0(b0) {}
};

template <typename A0>
inline emplace_args1<A0> create_emplace_args(BOOST_FWD_REF(A0) b0)
{
    emplace_args1<A0> e(b0);
    return e;
}

template <typename A0, typename A1> struct emplace_args2
{
    BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)
    BOOST_UNORDERED_EARGS_MEMBER(1, 1, _)

    emplace_args2(Arg0 b0, Arg1 b1) : a0(b0), a1(b1) {}
};

template <typename A0, typename A1>
inline emplace_args2<A0, A1> create_emplace_args(
    BOOST_FWD_REF(A0) b0, BOOST_FWD_REF(A1) b1)
{
    emplace_args2<A0, A1> e(b0, b1);
    return e;
}

template <typename A0, typename A1, typename A2> struct emplace_args3
{
    BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)
    BOOST_UNORDERED_EARGS_MEMBER(1, 1, _)
    BOOST_UNORDERED_EARGS_MEMBER(1, 2, _)

    emplace_args3(Arg0 b0, Arg1 b1, Arg2 b2) : a0(b0), a1(b1), a2(b2) {}
};

template <typename A0, typename A1, typename A2>
inline emplace_args3<A0, A1, A2> create_emplace_args(
    BOOST_FWD_REF(A0) b0, BOOST_FWD_REF(A1) b1, BOOST_FWD_REF(A2) b2)
{
    emplace_args3<A0, A1, A2> e(b0, b1, b2);
    return e;
}

#define BOOST_UNORDERED_FWD_PARAM(z, n, a)                                     \
    BOOST_FWD_REF(BOOST_PP_CAT(A, n)) BOOST_PP_CAT(a, n)

#define BOOST_UNORDERED_CALL_FORWARD(z, i, a)                                  \
    boost::forward<BOOST_PP_CAT(A, i)>(BOOST_PP_CAT(a, i))

#define BOOST_UNORDERED_EARGS_INIT(z, n, _)                                    \
    BOOST_PP_CAT(a, n)(BOOST_PP_CAT(b, n))

#define BOOST_UNORDERED_EARGS(z, n, _)                                         \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                        \
    struct BOOST_PP_CAT(emplace_args, n)                                       \
    {                                                                          \
        BOOST_PP_REPEAT_##z(n, BOOST_UNORDERED_EARGS_MEMBER, _) BOOST_PP_CAT(  \
            emplace_args, n)(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, Arg, b))      \
            : BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_EARGS_INIT, _)              \
        {                                                                      \
        }                                                                      \
    };                                                                         \
                                                                               \
    template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                        \
    inline BOOST_PP_CAT(emplace_args, n)<BOOST_PP_ENUM_PARAMS_Z(z, n, A)>      \
        create_emplace_args(                                                   \
            BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, b))                \
    {                                                                          \
        BOOST_PP_CAT(emplace_args, n)<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> e(      \
            BOOST_PP_ENUM_PARAMS_Z(z, n, b));                                  \
        return e;                                                              \
    }

BOOST_PP_REPEAT_FROM_TO(
    4, BOOST_UNORDERED_EMPLACE_LIMIT, BOOST_UNORDERED_EARGS, _)

#undef BOOST_UNORDERED_DEFINE_EMPLACE_ARGS
#undef BOOST_UNORDERED_EARGS_MEMBER
#undef BOOST_UNORDERED_EARGS_INIT

#endif

////////////////////////////////////////////////////////////////////////////////
//
// Some utilities for implementing allocator_traits, but useful elsewhere so
// they're always defined.

////////////////////////////////////////////////////////////////////////////
// Integral_constrant, true_type, false_type
//
// Uses the standard versions if available.

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

using std::integral_constant;
using std::true_type;
using std::false_type;

#else

template <typename T, T Value> struct integral_constant
{
    enum
    {
        value = Value
    };
};

typedef boost::unordered::detail::integral_constant<bool, true> true_type;
typedef boost::unordered::detail::integral_constant<bool, false> false_type;

#endif

////////////////////////////////////////////////////////////////////////////
// Explicitly call a destructor

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter
#endif

namespace func {
template <class T> inline void destroy(T* x) { x->~T(); }
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

////////////////////////////////////////////////////////////////////////////
// Expression test mechanism
//
// When SFINAE expressions are available, define
// BOOST_UNORDERED_HAS_FUNCTION which can check if a function call is
// supported by a class, otherwise define BOOST_UNORDERED_HAS_MEMBER which
// can detect if a class has the specified member, but not that it has the
// correct type, this is good enough for a passable impression of
// allocator_traits.

#if !defined(BOOST_NO_SFINAE_EXPR)

template <typename T, long unsigned int> struct expr_test;
template <typename T> struct expr_test<T, sizeof(char)> : T
{
};

#define BOOST_UNORDERED_CHECK_EXPRESSION(count, result, expression)            \
    template <typename U>                                                      \
    static typename boost::unordered::detail::expr_test<BOOST_PP_CAT(          \
                                                            choice, result),   \
        sizeof(for_expr_test(((expression), 0)))>::type                        \
        test(BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_DEFAULT_EXPRESSION(count, result)                      \
    template <typename U>                                                      \
    static BOOST_PP_CAT(choice, result)::type test(                            \
        BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_HAS_FUNCTION(name, thing, args, _)                     \
    struct BOOST_PP_CAT(has_, name)                                            \
    {                                                                          \
        template <typename U> static char for_expr_test(U const&);             \
        BOOST_UNORDERED_CHECK_EXPRESSION(                                      \
            1, 1, boost::unordered::detail::make<thing>().name args);          \
        BOOST_UNORDERED_DEFAULT_EXPRESSION(2, 2);                              \
                                                                               \
        enum                                                                   \
        {                                                                      \
            value = sizeof(test<T>(choose())) == sizeof(choice1::type)         \
        };                                                                     \
    }

#else

template <typename T> struct identity
{
    typedef T type;
};

#define BOOST_UNORDERED_CHECK_MEMBER(count, result, name, member)              \
                                                                               \
    typedef typename boost::unordered::detail::identity<member>::type          \
        BOOST_PP_CAT(check, count);                                            \
                                                                               \
    template <BOOST_PP_CAT(check, count) e> struct BOOST_PP_CAT(test, count)   \
    {                                                                          \
        typedef BOOST_PP_CAT(choice, result) type;                             \
    };                                                                         \
                                                                               \
    template <class U>                                                         \
    static typename BOOST_PP_CAT(test, count)<&U::name>::type test(            \
        BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_DEFAULT_MEMBER(count, result)                          \
    template <class U>                                                         \
    static BOOST_PP_CAT(choice, result)::type test(                            \
        BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_HAS_MEMBER(name)                                       \
    struct BOOST_PP_CAT(has_, name)                                            \
    {                                                                          \
        struct impl                                                            \
        {                                                                      \
            struct base_mixin                                                  \
            {                                                                  \
                int name;                                                      \
            };                                                                 \
            struct base : public T, public base_mixin                          \
            {                                                                  \
            };                                                                 \
                                                                               \
            BOOST_UNORDERED_CHECK_MEMBER(1, 1, name, int base_mixin::*);       \
            BOOST_UNORDERED_DEFAULT_MEMBER(2, 2);                              \
                                                                               \
            enum                                                               \
            {                                                                  \
                value = sizeof(choice2::type) == sizeof(test<base>(choose()))  \
            };                                                                 \
        };                                                                     \
                                                                               \
        enum                                                                   \
        {                                                                      \
            value = impl::value                                                \
        };                                                                     \
    }

#endif
}
}
}

////////////////////////////////////////////////////////////////////////////////
//
// Allocator traits
//
// First our implementation, then later light wrappers around the alternatives

#if BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 0

#include <boost/limits.hpp>
#include <boost/pointer_to_other.hpp>
#include <boost/utility/enable_if.hpp>

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) &&                             \
    !defined(BOOST_NO_SFINAE_EXPR)
#define BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT 1
#else
#define BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT 0
#endif

namespace boost {
namespace unordered {
namespace detail {

template <typename Alloc, typename T> struct rebind_alloc;

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template <template <typename, typename...> class Alloc, typename U, typename T,
    typename... Args>
struct rebind_alloc<Alloc<U, Args...>, T>
{
    typedef Alloc<T, Args...> type;
};

#else

template <template <typename> class Alloc, typename U, typename T>
struct rebind_alloc<Alloc<U>, T>
{
    typedef Alloc<T> type;
};

template <template <typename, typename> class Alloc, typename U, typename T,
    typename A0>
struct rebind_alloc<Alloc<U, A0>, T>
{
    typedef Alloc<T, A0> type;
};

template <template <typename, typename, typename> class Alloc, typename U,
    typename T, typename A0, typename A1>
struct rebind_alloc<Alloc<U, A0, A1>, T>
{
    typedef Alloc<T, A0, A1> type;
};

#endif

template <typename Alloc, typename T> struct rebind_wrap
{
    template <typename X>
    static choice1::type test(
        choice1, typename X::BOOST_NESTED_TEMPLATE rebind<T>::other* = 0);
    template <typename X> static choice2::type test(choice2, void* = 0);

    enum
    {
        value = (1 == sizeof(test<Alloc>(choose())))
    };

    struct fallback
    {
        template <typename U> struct rebind
        {
            typedef typename rebind_alloc<Alloc, T>::type other;
        };
    };

    typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE then<
        Alloc, fallback>::type::BOOST_NESTED_TEMPLATE rebind<T>::other type;
};

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1400

#define BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(tname)                              \
    template <typename Tp, typename Default> struct default_type_##tname       \
    {                                                                          \
                                                                               \
        template <typename X>                                                  \
        static choice1::type test(choice1, typename X::tname* = 0);            \
                                                                               \
        template <typename X> static choice2::type test(choice2, void* = 0);   \
                                                                               \
        struct DefaultWrap                                                     \
        {                                                                      \
            typedef Default tname;                                             \
        };                                                                     \
                                                                               \
        enum                                                                   \
        {                                                                      \
            value = (1 == sizeof(test<Tp>(choose())))                          \
        };                                                                     \
                                                                               \
        typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE  \
            then<Tp, DefaultWrap>::type::tname type;                           \
    }

#else

template <typename T, typename T2> struct sfinae : T2
{
};

#define BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(tname)                              \
    template <typename Tp, typename Default> struct default_type_##tname       \
    {                                                                          \
                                                                               \
        template <typename X>                                                  \
        static typename boost::unordered::detail::sfinae<typename X::tname,    \
            choice1>::type test(choice1);                                      \
                                                                               \
        template <typename X> static choice2::type test(choice2);              \
                                                                               \
        struct DefaultWrap                                                     \
        {                                                                      \
            typedef Default tname;                                             \
        };                                                                     \
                                                                               \
        enum                                                                   \
        {                                                                      \
            value = (1 == sizeof(test<Tp>(choose())))                          \
        };                                                                     \
                                                                               \
        typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE  \
            then<Tp, DefaultWrap>::type::tname type;                           \
    }

#endif

#define BOOST_UNORDERED_DEFAULT_TYPE(T, tname, arg)                            \
    typename default_type_##tname<T, arg>::type

BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(pointer);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(const_pointer);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(void_pointer);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(const_void_pointer);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(difference_type);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(size_type);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(propagate_on_container_copy_assignment);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(propagate_on_container_move_assignment);
BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(propagate_on_container_swap);

#if !defined(BOOST_NO_SFINAE_EXPR)

template <typename T>
BOOST_UNORDERED_HAS_FUNCTION(
    select_on_container_copy_construction, U const, (), 0);

template <typename T> BOOST_UNORDERED_HAS_FUNCTION(max_size, U const, (), 0);

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template <typename T, typename ValueType, typename... Args>
BOOST_UNORDERED_HAS_FUNCTION(
    construct, U, (boost::unordered::detail::make<ValueType*>(),
                      boost::unordered::detail::make<Args const>()...),
    2);

#else

template <typename T, typename ValueType>
BOOST_UNORDERED_HAS_FUNCTION(
    construct, U, (boost::unordered::detail::make<ValueType*>(),
                      boost::unordered::detail::make<ValueType const>()),
    2);

#endif

template <typename T, typename ValueType>
BOOST_UNORDERED_HAS_FUNCTION(
    destroy, U, (boost::unordered::detail::make<ValueType*>()), 1);

#else

template <typename T>
BOOST_UNORDERED_HAS_MEMBER(select_on_container_copy_construction);

template <typename T> BOOST_UNORDERED_HAS_MEMBER(max_size);

template <typename T, typename ValueType> BOOST_UNORDERED_HAS_MEMBER(construct);

template <typename T, typename ValueType> BOOST_UNORDERED_HAS_MEMBER(destroy);

#endif

namespace func {

template <typename Alloc>
inline Alloc call_select_on_container_copy_construction(const Alloc& rhs,
    typename boost::enable_if_c<
        boost::unordered::detail::has_select_on_container_copy_construction<
            Alloc>::value,
        void*>::type = 0)
{
    return rhs.select_on_container_copy_construction();
}

template <typename Alloc>
inline Alloc call_select_on_container_copy_construction(const Alloc& rhs,
    typename boost::disable_if_c<
        boost::unordered::detail::has_select_on_container_copy_construction<
            Alloc>::value,
        void*>::type = 0)
{
    return rhs;
}

template <typename SizeType, typename Alloc>
inline SizeType call_max_size(const Alloc& a,
    typename boost::enable_if_c<
        boost::unordered::detail::has_max_size<Alloc>::value, void*>::type = 0)
{
    return a.max_size();
}

template <typename SizeType, typename Alloc>
inline SizeType call_max_size(const Alloc&,
    typename boost::disable_if_c<
        boost::unordered::detail::has_max_size<Alloc>::value, void*>::type = 0)
{
    return (std::numeric_limits<SizeType>::max)();
}

} // namespace func.

template <typename Alloc> struct allocator_traits
{
    typedef Alloc allocator_type;
    typedef typename Alloc::value_type value_type;

    typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, pointer, value_type*) pointer;

    template <typename T>
    struct pointer_to_other : boost::pointer_to_other<pointer, T>
    {
    };

    typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, const_pointer,
        typename pointer_to_other<const value_type>::type) const_pointer;

    // typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, void_pointer,
    //    typename pointer_to_other<void>::type)
    //    void_pointer;
    //
    // typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, const_void_pointer,
    //    typename pointer_to_other<const void>::type)
    //    const_void_pointer;

    typedef BOOST_UNORDERED_DEFAULT_TYPE(
        Alloc, difference_type, std::ptrdiff_t) difference_type;

    typedef BOOST_UNORDERED_DEFAULT_TYPE(
        Alloc, size_type, std::size_t) size_type;

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
    template <typename T>
    using rebind_alloc = typename rebind_wrap<Alloc, T>::type;

    template <typename T>
    using rebind_traits =
        boost::unordered::detail::allocator_traits<rebind_alloc<T> >;
#endif

    static pointer allocate(Alloc& a, size_type n) { return a.allocate(n); }

    // I never use this, so I'll just comment it out for now.
    //
    // static pointer allocate(Alloc& a, size_type n,
    //        const_void_pointer hint)
    //    { return DEFAULT_FUNC(allocate, pointer)(a, n, hint); }

    static void deallocate(Alloc& a, pointer p, size_type n)
    {
        a.deallocate(p, n);
    }

  public:
#if BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT

    template <typename T, typename... Args>
    static typename boost::enable_if_c<
        boost::unordered::detail::has_construct<Alloc, T, Args...>::value>::type
    construct(Alloc& a, T* p, BOOST_FWD_REF(Args)... x)
    {
        a.construct(p, boost::forward<Args>(x)...);
    }

    template <typename T, typename... Args>
    static typename boost::disable_if_c<
        boost::unordered::detail::has_construct<Alloc, T, Args...>::value>::type
    construct(Alloc&, T* p, BOOST_FWD_REF(Args)... x)
    {
        new (static_cast<void*>(p)) T(boost::forward<Args>(x)...);
    }

    template <typename T>
    static typename boost::enable_if_c<
        boost::unordered::detail::has_destroy<Alloc, T>::value>::type
    destroy(Alloc& a, T* p)
    {
        a.destroy(p);
    }

    template <typename T>
    static typename boost::disable_if_c<
        boost::unordered::detail::has_destroy<Alloc, T>::value>::type
    destroy(Alloc&, T* p)
    {
        boost::unordered::detail::func::destroy(p);
    }

#elif !defined(BOOST_NO_SFINAE_EXPR)

    template <typename T>
    static typename boost::enable_if_c<
        boost::unordered::detail::has_construct<Alloc, T>::value>::type
    construct(Alloc& a, T* p, T const& x)
    {
        a.construct(p, x);
    }

    template <typename T>
    static typename boost::disable_if_c<
        boost::unordered::detail::has_construct<Alloc, T>::value>::type
    construct(Alloc&, T* p, T const& x)
    {
        new (static_cast<void*>(p)) T(x);
    }

    template <typename T>
    static typename boost::enable_if_c<
        boost::unordered::detail::has_destroy<Alloc, T>::value>::type
    destroy(Alloc& a, T* p)
    {
        a.destroy(p);
    }

    template <typename T>
    static typename boost::disable_if_c<
        boost::unordered::detail::has_destroy<Alloc, T>::value>::type
    destroy(Alloc&, T* p)
    {
        boost::unordered::detail::func::destroy(p);
    }

#else

    // If we don't have SFINAE expressions, only call construct for the
    // copy constructor for the allocator's value_type - as that's
    // the only construct method that old fashioned allocators support.

    template <typename T>
    static void construct(Alloc& a, T* p, T const& x,
        typename boost::enable_if_c<
            boost::unordered::detail::has_construct<Alloc, T>::value &&
                boost::is_same<T, value_type>::value,
            void*>::type = 0)
    {
        a.construct(p, x);
    }

    template <typename T>
    static void construct(Alloc&, T* p, T const& x,
        typename boost::disable_if_c<
            boost::unordered::detail::has_construct<Alloc, T>::value &&
                boost::is_same<T, value_type>::value,
            void*>::type = 0)
    {
        new (static_cast<void*>(p)) T(x);
    }

    template <typename T>
    static void destroy(Alloc& a, T* p,
        typename boost::enable_if_c<
            boost::unordered::detail::has_destroy<Alloc, T>::value &&
                boost::is_same<T, value_type>::value,
            void*>::type = 0)
    {
        a.destroy(p);
    }

    template <typename T>
    static void destroy(Alloc&, T* p,
        typename boost::disable_if_c<
            boost::unordered::detail::has_destroy<Alloc, T>::value &&
                boost::is_same<T, value_type>::value,
            void*>::type = 0)
    {
        boost::unordered::detail::func::destroy(p);
    }

#endif

    static size_type max_size(const Alloc& a)
    {
        return boost::unordered::detail::func::call_max_size<size_type>(a);
    }

    // Allocator propagation on construction

    static Alloc select_on_container_copy_construction(Alloc const& rhs)
    {
        return boost::unordered::detail::func::
            call_select_on_container_copy_construction(rhs);
    }

    // Allocator propagation on assignment and swap.
    // Return true if lhs is modified.
    typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc,
        propagate_on_container_copy_assignment,
        false_type) propagate_on_container_copy_assignment;
    typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc,
        propagate_on_container_move_assignment,
        false_type) propagate_on_container_move_assignment;
    typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, propagate_on_container_swap,
        false_type) propagate_on_container_swap;
};
}
}
}

#undef BOOST_UNORDERED_DEFAULT_TYPE_TMPLT
#undef BOOST_UNORDERED_DEFAULT_TYPE

////////////////////////////////////////////////////////////////////////////////
//
// std::allocator_traits

#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1

#include <memory>

#define BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT 1

namespace boost {
namespace unordered {
namespace detail {

template <typename Alloc> struct allocator_traits : std::allocator_traits<Alloc>
{
};

template <typename Alloc, typename T> struct rebind_wrap
{
    typedef typename std::allocator_traits<Alloc>::template rebind_alloc<T>
        type;
};
}
}
}

////////////////////////////////////////////////////////////////////////////////
//
// boost::container::allocator_traits

#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 2

#include <boost/container/allocator_traits.hpp>

#define BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT 0

namespace boost {
namespace unordered {
namespace detail {

template <typename Alloc>
struct allocator_traits : boost::container::allocator_traits<Alloc>
{
};

template <typename Alloc, typename T>
struct rebind_wrap : boost::container::allocator_traits<
                         Alloc>::template portable_rebind_alloc<T>
{
};
}
}
}

#else

#error "Invalid BOOST_UNORDERED_USE_ALLOCATOR_TRAITS value."

#endif

////////////////////////////////////////////////////////////////////////////
// Functions used to construct nodes. Emulates variadic construction,
// piecewise construction etc.

namespace boost {
namespace unordered {
namespace detail {
namespace func {

////////////////////////////////////////////////////////////////////////////
// call_construct

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#if BOOST_UNORDERED_DETAIL_FULL_CONSTRUCT

template <typename Alloc, typename T, typename... Args>
inline void call_construct(
    Alloc& alloc, T* address, BOOST_FWD_REF(Args)... args)
{
    boost::unordered::detail::allocator_traits<Alloc>::construct(
        alloc, address, boost::forward<Args>(args)...);
}

template <typename Alloc, typename T>
inline void call_destroy(Alloc& alloc, T* x)
{
    boost::unordered::detail::allocator_traits<Alloc>::destroy(alloc, x);
}

#else

template <typename Alloc, typename T, typename... Args>
inline void call_construct(Alloc&, T* address, BOOST_FWD_REF(Args)... args)
{
    new ((void*)address) T(boost::forward<Args>(args)...);
}

template <typename Alloc, typename T> inline void call_destroy(Alloc&, T* x)
{
    boost::unordered::detail::func::destroy(x);
}

#endif

#else
template <typename Alloc, typename T>
inline void call_construct(Alloc&, T* address)
{
    new ((void*)address) T();
}

template <typename Alloc, typename T, typename A0>
inline void call_construct(Alloc&, T* address, BOOST_FWD_REF(A0) a0)
{
    new ((void*)address) T(boost::forward<A0>(a0));
}

template <typename Alloc, typename T> inline void call_destroy(Alloc&, T* x)
{
    boost::unordered::detail::func::destroy(x);
}

#endif

////////////////////////////////////////////////////////////////////////////
// Construct from tuple
//
// Used for piecewise construction.

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(n, namespace_)                    \
    template <typename Alloc, typename T>                                      \
    void construct_from_tuple(Alloc& alloc, T* ptr, namespace_ tuple<>)        \
    {                                                                          \
        boost::unordered::detail::func::call_construct(alloc, ptr);            \
    }                                                                          \
                                                                               \
    BOOST_PP_REPEAT_FROM_TO(                                                   \
        1, n, BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL, namespace_)

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL(z, n, namespace_)            \
    template <typename Alloc, typename T,                                      \
        BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                              \
    void construct_from_tuple(Alloc& alloc, T* ptr,                            \
        namespace_ tuple<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> const& x)            \
    {                                                                          \
        boost::unordered::detail::func::call_construct(alloc, ptr,             \
            BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_GET_TUPLE_ARG, namespace_));  \
    }

#define BOOST_UNORDERED_GET_TUPLE_ARG(z, n, namespace_) namespace_ get<n>(x)

#elif !defined(__SUNPRO_CC)

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(n, namespace_)                    \
    template <typename Alloc, typename T>                                      \
    void construct_from_tuple(Alloc&, T* ptr, namespace_ tuple<>)              \
    {                                                                          \
        new ((void*)ptr) T();                                                  \
    }                                                                          \
                                                                               \
    BOOST_PP_REPEAT_FROM_TO(                                                   \
        1, n, BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL, namespace_)

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL(z, n, namespace_)            \
    template <typename Alloc, typename T,                                      \
        BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                              \
    void construct_from_tuple(Alloc&, T* ptr,                                  \
        namespace_ tuple<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> const& x)            \
    {                                                                          \
        new ((void*)ptr) T(                                                    \
            BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_GET_TUPLE_ARG, namespace_));  \
    }

#define BOOST_UNORDERED_GET_TUPLE_ARG(z, n, namespace_) namespace_ get<n>(x)

#else

template <int N> struct length
{
};

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(n, namespace_)                    \
    template <typename Alloc, typename T>                                      \
    void construct_from_tuple_impl(boost::unordered::detail::func::length<0>,  \
        Alloc&, T* ptr, namespace_ tuple<>)                                    \
    {                                                                          \
        new ((void*)ptr) T();                                                  \
    }                                                                          \
                                                                               \
    BOOST_PP_REPEAT_FROM_TO(                                                   \
        1, n, BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL, namespace_)

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL(z, n, namespace_)            \
    template <typename Alloc, typename T,                                      \
        BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                              \
    void construct_from_tuple_impl(boost::unordered::detail::func::length<n>,  \
        Alloc&, T* ptr,                                                        \
        namespace_ tuple<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> const& x)            \
    {                                                                          \
        new ((void*)ptr) T(                                                    \
            BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_GET_TUPLE_ARG, namespace_));  \
    }

#define BOOST_UNORDERED_GET_TUPLE_ARG(z, n, namespace_) namespace_ get<n>(x)

#endif

BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(10, boost::)

#if !defined(__SUNPRO_CC) && !defined(BOOST_NO_CXX11_HDR_TUPLE)
BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(10, std::)
#endif

#undef BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE
#undef BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE_IMPL
#undef BOOST_UNORDERED_GET_TUPLE_ARG

#if defined(__SUNPRO_CC)

template <typename Alloc, typename T, typename Tuple>
void construct_from_tuple(Alloc& alloc, T* ptr, Tuple const& x)
{
    construct_from_tuple_impl(boost::unordered::detail::func::length<
                                  boost::tuples::length<Tuple>::value>(),
        alloc, ptr, x);
}

#endif

////////////////////////////////////////////////////////////////////////////
// Trait to check for piecewise construction.

template <typename A0> struct use_piecewise
{
    static choice1::type test(choice1, boost::unordered::piecewise_construct_t);

    static choice2::type test(choice2, ...);

    enum
    {
        value = sizeof(choice1::type) ==
                sizeof(test(choose(), boost::unordered::detail::make<A0>()))
    };
};

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

////////////////////////////////////////////////////////////////////////////
// Construct from variadic parameters

// For the standard pair constructor.

template <typename Alloc, typename T, typename... Args>
inline void construct_from_args(
    Alloc& alloc, T* address, BOOST_FWD_REF(Args)... args)
{
    boost::unordered::detail::func::call_construct(
        alloc, address, boost::forward<Args>(args)...);
}

// Special case for piece_construct
//
// TODO: When possible, it might be better to use std::pair's
// constructor for std::piece_construct with std::tuple.

template <typename Alloc, typename A, typename B, typename A0, typename A1,
    typename A2>
inline typename enable_if<use_piecewise<A0>, void>::type construct_from_args(
    Alloc& alloc, std::pair<A, B>* address, BOOST_FWD_REF(A0),
    BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
{
    boost::unordered::detail::func::construct_from_tuple(
        alloc, boost::unordered::detail::func::const_cast_pointer(
                   boost::addressof(address->first)),
        boost::forward<A1>(a1));
    BOOST_TRY
    {
        boost::unordered::detail::func::construct_from_tuple(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(address->second)),
            boost::forward<A2>(a2));
    }
    BOOST_CATCH(...)
    {
        boost::unordered::detail::func::call_destroy(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(address->first)));
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
}

#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

////////////////////////////////////////////////////////////////////////////
// Construct from emplace_args

// Explicitly write out first three overloads for the sake of sane
// error messages.

template <typename Alloc, typename T, typename A0>
inline void construct_from_args(
    Alloc&, T* address, emplace_args1<A0> const& args)
{
    new ((void*)address) T(boost::forward<A0>(args.a0));
}

template <typename Alloc, typename T, typename A0, typename A1>
inline void construct_from_args(
    Alloc&, T* address, emplace_args2<A0, A1> const& args)
{
    new ((void*)address)
        T(boost::forward<A0>(args.a0), boost::forward<A1>(args.a1));
}

template <typename Alloc, typename T, typename A0, typename A1, typename A2>
inline void construct_from_args(
    Alloc&, T* address, emplace_args3<A0, A1, A2> const& args)
{
    new ((void*)address) T(boost::forward<A0>(args.a0),
        boost::forward<A1>(args.a1), boost::forward<A2>(args.a2));
}

// Use a macro for the rest.

#define BOOST_UNORDERED_CONSTRUCT_IMPL(z, num_params, _)                       \
    template <typename Alloc, typename T,                                      \
        BOOST_PP_ENUM_PARAMS_Z(z, num_params, typename A)>                     \
    inline void construct_from_args(Alloc&, T* address,                        \
        boost::unordered::detail::BOOST_PP_CAT(emplace_args, num_params) <     \
            BOOST_PP_ENUM_PARAMS_Z(z, num_params, A) > const& args)            \
    {                                                                          \
        new ((void*)address) T(BOOST_PP_ENUM_##z(                              \
            num_params, BOOST_UNORDERED_CALL_FORWARD, args.a));                \
    }

BOOST_PP_REPEAT_FROM_TO(
    4, BOOST_UNORDERED_EMPLACE_LIMIT, BOOST_UNORDERED_CONSTRUCT_IMPL, _)

#undef BOOST_UNORDERED_CONSTRUCT_IMPL

// Construct with piece_construct

template <typename Alloc, typename A, typename B, typename A0, typename A1,
    typename A2>
inline void construct_from_args(Alloc& alloc, std::pair<A, B>* address,
    boost::unordered::detail::emplace_args3<A0, A1, A2> const& args,
    typename enable_if<use_piecewise<A0>, void*>::type = 0)
{
    boost::unordered::detail::func::construct_from_tuple(
        alloc, boost::unordered::detail::func::const_cast_pointer(
                   boost::addressof(address->first)),
        args.a1);
    BOOST_TRY
    {
        boost::unordered::detail::func::construct_from_tuple(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(address->second)),
            args.a2);
    }
    BOOST_CATCH(...)
    {
        boost::unordered::detail::func::call_destroy(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(address->first)));
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
}

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES
}
}
}
}

namespace boost {
namespace unordered {
namespace detail {

///////////////////////////////////////////////////////////////////
//
// Node construction

template <typename NodeAlloc> struct node_constructor
{
    typedef NodeAlloc node_allocator;
    typedef boost::unordered::detail::allocator_traits<NodeAlloc>
        node_allocator_traits;
    typedef typename node_allocator_traits::value_type node;
    typedef typename node_allocator_traits::pointer node_pointer;
    typedef typename node::value_type value_type;

    node_allocator& alloc_;
    node_pointer node_;
    bool node_constructed_;

    node_constructor(node_allocator& n)
        : alloc_(n), node_(), node_constructed_(false)
    {
    }

    ~node_constructor();

    void create_node();

    // no throw
    node_pointer release()
    {
        BOOST_ASSERT(node_ && node_constructed_);
        node_pointer p = node_;
        node_ = node_pointer();
        return p;
    }

    void reclaim(node_pointer p)
    {
        BOOST_ASSERT(!node_);
        node_ = p;
        node_constructed_ = true;
        boost::unordered::detail::func::call_destroy(
            alloc_, node_->value_ptr());
    }

  private:
    node_constructor(node_constructor const&);
    node_constructor& operator=(node_constructor const&);
};

template <typename Alloc> node_constructor<Alloc>::~node_constructor()
{
    if (node_) {
        if (node_constructed_) {
            boost::unordered::detail::func::destroy(boost::addressof(*node_));
        }

        node_allocator_traits::deallocate(alloc_, node_, 1);
    }
}

template <typename Alloc> void node_constructor<Alloc>::create_node()
{
    BOOST_ASSERT(!node_);
    node_constructed_ = false;

    node_ = node_allocator_traits::allocate(alloc_, 1);

    new ((void*)boost::addressof(*node_)) node();
    node_->init(node_);
    node_constructed_ = true;
}

template <typename NodeAlloc> struct node_tmp
{
    typedef boost::unordered::detail::allocator_traits<NodeAlloc>
        node_allocator_traits;
    typedef typename node_allocator_traits::pointer node_pointer;

    NodeAlloc& alloc_;
    node_pointer node_;

    explicit node_tmp(node_pointer n, NodeAlloc& a) : alloc_(a), node_(n) {}

    ~node_tmp();

    // no throw
    node_pointer release()
    {
        node_pointer p = node_;
        node_ = node_pointer();
        return p;
    }
};

template <typename Alloc> node_tmp<Alloc>::~node_tmp()
{
    if (node_) {
        boost::unordered::detail::func::call_destroy(
            alloc_, node_->value_ptr());
        boost::unordered::detail::func::destroy(boost::addressof(*node_));
        node_allocator_traits::deallocate(alloc_, node_, 1);
    }
}
}
}
}

namespace boost {
namespace unordered {
namespace detail {
namespace func {

// Some nicer construct_node functions, might try to
// improve implementation later.

template <typename Alloc, BOOST_UNORDERED_EMPLACE_TEMPLATE>
inline typename boost::unordered::detail::allocator_traits<Alloc>::pointer
construct_node_from_args(Alloc& alloc, BOOST_UNORDERED_EMPLACE_ARGS)
{
    node_constructor<Alloc> a(alloc);
    a.create_node();
    construct_from_args(
        alloc, a.node_->value_ptr(), BOOST_UNORDERED_EMPLACE_FORWARD);
    return a.release();
}

template <typename Alloc, typename U>
inline typename boost::unordered::detail::allocator_traits<Alloc>::pointer
construct_node(Alloc& alloc, BOOST_FWD_REF(U) x)
{
    node_constructor<Alloc> a(alloc);
    a.create_node();
    boost::unordered::detail::func::call_construct(
        alloc, a.node_->value_ptr(), boost::forward<U>(x));
    return a.release();
}

// TODO: When possible, it might be better to use std::pair's
// constructor for std::piece_construct with std::tuple.
template <typename Alloc, typename Key>
inline typename boost::unordered::detail::allocator_traits<Alloc>::pointer
construct_node_pair(Alloc& alloc, BOOST_FWD_REF(Key) k)
{
    node_constructor<Alloc> a(alloc);
    a.create_node();
    boost::unordered::detail::func::call_construct(
        alloc, boost::unordered::detail::func::const_cast_pointer(
                   boost::addressof(a.node_->value_ptr()->first)),
        boost::forward<Key>(k));
    BOOST_TRY
    {
        boost::unordered::detail::func::call_construct(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->second)));
    }
    BOOST_CATCH(...)
    {
        boost::unordered::detail::func::call_destroy(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->first)));
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
    return a.release();
}

template <typename Alloc, typename Key, typename Mapped>
inline typename boost::unordered::detail::allocator_traits<Alloc>::pointer
construct_node_pair(Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_FWD_REF(Mapped) m)
{
    node_constructor<Alloc> a(alloc);
    a.create_node();
    boost::unordered::detail::func::call_construct(
        alloc, boost::unordered::detail::func::const_cast_pointer(
                   boost::addressof(a.node_->value_ptr()->first)),
        boost::forward<Key>(k));
    BOOST_TRY
    {
        boost::unordered::detail::func::call_construct(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->second)),
            boost::forward<Mapped>(m));
    }
    BOOST_CATCH(...)
    {
        boost::unordered::detail::func::call_destroy(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->first)));
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
    return a.release();
}

template <typename Alloc, typename Key, BOOST_UNORDERED_EMPLACE_TEMPLATE>
inline typename boost::unordered::detail::allocator_traits<Alloc>::pointer
construct_node_pair_from_args(
    Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
{
    node_constructor<Alloc> a(alloc);
    a.create_node();
    boost::unordered::detail::func::call_construct(
        alloc, boost::unordered::detail::func::const_cast_pointer(
                   boost::addressof(a.node_->value_ptr()->first)),
        boost::forward<Key>(k));
    BOOST_TRY
    {
        boost::unordered::detail::func::construct_from_args(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->second)),
            BOOST_UNORDERED_EMPLACE_FORWARD);
    }
    BOOST_CATCH(...)
    {
        boost::unordered::detail::func::call_destroy(
            alloc, boost::unordered::detail::func::const_cast_pointer(
                       boost::addressof(a.node_->value_ptr()->first)));
        BOOST_RETHROW;
    }
    BOOST_CATCH_END
    return a.release();
}
}
}
}
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

// The 'iterator_detail' namespace was a misguided attempt at avoiding ADL
// in the detail namespace. It didn't work because the template parameters
// were in detail. I'm not changing it at the moment to be safe. I might
// do in the future if I change the iterator types.
namespace boost {
namespace unordered {
namespace iterator_detail {

////////////////////////////////////////////////////////////////////////////
// Iterators
//
// all no throw

template <typename Node, typename Policy>
struct l_iterator : public std::iterator<std::forward_iterator_tag,
                        typename Node::value_type, std::ptrdiff_t,
                        typename Node::value_type*, typename Node::value_type&>
{
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template <typename Node2, typename Policy2>
    friend struct boost::unordered::iterator_detail::cl_iterator;

  private:
#endif
    typedef typename Node::node_pointer node_pointer;
    node_pointer ptr_;
    std::size_t bucket_;
    std::size_t bucket_count_;

  public:
    typedef typename Node::value_type value_type;

    l_iterator() BOOST_NOEXCEPT : ptr_() {}

    l_iterator(node_pointer n, std::size_t b, std::size_t c) BOOST_NOEXCEPT
        : ptr_(n),
          bucket_(b),
          bucket_count_(c)
    {
    }

    value_type& operator*() const { return ptr_->value(); }

    value_type* operator->() const { return ptr_->value_ptr(); }

    l_iterator& operator++()
    {
        ptr_ = static_cast<node_pointer>(ptr_->next_);
        if (ptr_ && Policy::to_bucket(bucket_count_, ptr_->hash_) != bucket_)
            ptr_ = node_pointer();
        return *this;
    }

    l_iterator operator++(int)
    {
        l_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(l_iterator x) const BOOST_NOEXCEPT
    {
        return ptr_ == x.ptr_;
    }

    bool operator!=(l_iterator x) const BOOST_NOEXCEPT
    {
        return ptr_ != x.ptr_;
    }
};

template <typename Node, typename Policy>
struct cl_iterator
    : public std::iterator<std::forward_iterator_tag, typename Node::value_type,
          std::ptrdiff_t, typename Node::value_type const*,
          typename Node::value_type const&>
{
    friend struct boost::unordered::iterator_detail::l_iterator<Node, Policy>;

  private:
    typedef typename Node::node_pointer node_pointer;
    node_pointer ptr_;
    std::size_t bucket_;
    std::size_t bucket_count_;

  public:
    typedef typename Node::value_type value_type;

    cl_iterator() BOOST_NOEXCEPT : ptr_() {}

    cl_iterator(node_pointer n, std::size_t b, std::size_t c) BOOST_NOEXCEPT
        : ptr_(n),
          bucket_(b),
          bucket_count_(c)
    {
    }

    cl_iterator(
        boost::unordered::iterator_detail::l_iterator<Node, Policy> const& x)
        BOOST_NOEXCEPT : ptr_(x.ptr_),
                         bucket_(x.bucket_),
                         bucket_count_(x.bucket_count_)
    {
    }

    value_type const& operator*() const { return ptr_->value(); }

    value_type const* operator->() const { return ptr_->value_ptr(); }

    cl_iterator& operator++()
    {
        ptr_ = static_cast<node_pointer>(ptr_->next_);
        if (ptr_ && Policy::to_bucket(bucket_count_, ptr_->hash_) != bucket_)
            ptr_ = node_pointer();
        return *this;
    }

    cl_iterator operator++(int)
    {
        cl_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    friend bool operator==(
        cl_iterator const& x, cl_iterator const& y) BOOST_NOEXCEPT
    {
        return x.ptr_ == y.ptr_;
    }

    friend bool operator!=(
        cl_iterator const& x, cl_iterator const& y) BOOST_NOEXCEPT
    {
        return x.ptr_ != y.ptr_;
    }
};

template <typename Node>
struct iterator : public std::iterator<std::forward_iterator_tag,
                      typename Node::value_type, std::ptrdiff_t,
                      typename Node::value_type*, typename Node::value_type&>
{
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template <typename>
    friend struct boost::unordered::iterator_detail::c_iterator;
    template <typename> friend struct boost::unordered::detail::table;
    template <typename> friend struct boost::unordered::detail::table_impl;
    template <typename>
    friend struct boost::unordered::detail::grouped_table_impl;

  private:
#endif
    typedef typename Node::node_pointer node_pointer;
    node_pointer node_;

  public:
    typedef typename Node::value_type value_type;

    iterator() BOOST_NOEXCEPT : node_() {}

    explicit iterator(typename Node::link_pointer x) BOOST_NOEXCEPT
        : node_(static_cast<node_pointer>(x))
    {
    }

    value_type& operator*() const { return node_->value(); }

    value_type* operator->() const { return node_->value_ptr(); }

    iterator& operator++()
    {
        node_ = static_cast<node_pointer>(node_->next_);
        return *this;
    }

    iterator operator++(int)
    {
        iterator tmp(node_);
        node_ = static_cast<node_pointer>(node_->next_);
        return tmp;
    }

    bool operator==(iterator const& x) const BOOST_NOEXCEPT
    {
        return node_ == x.node_;
    }

    bool operator!=(iterator const& x) const BOOST_NOEXCEPT
    {
        return node_ != x.node_;
    }
};

template <typename Node>
struct c_iterator
    : public std::iterator<std::forward_iterator_tag, typename Node::value_type,
          std::ptrdiff_t, typename Node::value_type const*,
          typename Node::value_type const&>
{
    friend struct boost::unordered::iterator_detail::iterator<Node>;

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
    template <typename> friend struct boost::unordered::detail::table;
    template <typename> friend struct boost::unordered::detail::table_impl;
    template <typename>
    friend struct boost::unordered::detail::grouped_table_impl;

  private:
#endif
    typedef typename Node::node_pointer node_pointer;
    typedef boost::unordered::iterator_detail::iterator<Node> n_iterator;
    node_pointer node_;

  public:
    typedef typename Node::value_type value_type;

    c_iterator() BOOST_NOEXCEPT : node_() {}

    explicit c_iterator(typename Node::link_pointer x) BOOST_NOEXCEPT
        : node_(static_cast<node_pointer>(x))
    {
    }

    c_iterator(n_iterator const& x) BOOST_NOEXCEPT : node_(x.node_) {}

    value_type const& operator*() const { return node_->value(); }

    value_type const* operator->() const { return node_->value_ptr(); }

    c_iterator& operator++()
    {
        node_ = static_cast<node_pointer>(node_->next_);
        return *this;
    }

    c_iterator operator++(int)
    {
        c_iterator tmp(node_);
        node_ = static_cast<node_pointer>(node_->next_);
        return tmp;
    }

    friend bool operator==(
        c_iterator const& x, c_iterator const& y) BOOST_NOEXCEPT
    {
        return x.node_ == y.node_;
    }

    friend bool operator!=(
        c_iterator const& x, c_iterator const& y) BOOST_NOEXCEPT
    {
        return x.node_ != y.node_;
    }
};
}
}
}

namespace boost {
namespace unordered {
namespace detail {

///////////////////////////////////////////////////////////////////
//
// Node Holder
//
// Temporary store for nodes. Deletes any that aren't used.

template <typename NodeAlloc> struct node_holder
{
  private:
    typedef NodeAlloc node_allocator;
    typedef boost::unordered::detail::allocator_traits<NodeAlloc>
        node_allocator_traits;
    typedef typename node_allocator_traits::value_type node;
    typedef typename node_allocator_traits::pointer node_pointer;
    typedef typename node::value_type value_type;
    typedef typename node::link_pointer link_pointer;
    typedef boost::unordered::iterator_detail::iterator<node> iterator;

    node_constructor<NodeAlloc> constructor_;
    node_pointer nodes_;

  public:
    template <typename Table>
    explicit node_holder(Table& b) : constructor_(b.node_alloc()), nodes_()
    {
        if (b.size_) {
            typename Table::link_pointer prev = b.get_previous_start();
            nodes_ = static_cast<node_pointer>(prev->next_);
            prev->next_ = link_pointer();
            b.size_ = 0;
        }
    }

    ~node_holder();

    node_pointer pop_node()
    {
        node_pointer n = nodes_;
        nodes_ = static_cast<node_pointer>(nodes_->next_);
        n->init(n);
        n->next_ = link_pointer();
        return n;
    }

    template <typename T> inline node_pointer copy_of(T const& v)
    {
        if (nodes_) {
            constructor_.reclaim(pop_node());
        } else {
            constructor_.create_node();
        }
        boost::unordered::detail::func::call_construct(
            constructor_.alloc_, constructor_.node_->value_ptr(), v);
        return constructor_.release();
    }

    template <typename T> inline node_pointer move_copy_of(T& v)
    {
        if (nodes_) {
            constructor_.reclaim(pop_node());
        } else {
            constructor_.create_node();
        }
        boost::unordered::detail::func::call_construct(constructor_.alloc_,
            constructor_.node_->value_ptr(), boost::move(v));
        return constructor_.release();
    }

    iterator begin() const { return iterator(nodes_); }
};

template <typename Alloc> node_holder<Alloc>::~node_holder()
{
    while (nodes_) {
        node_pointer p = nodes_;
        nodes_ = static_cast<node_pointer>(p->next_);

        boost::unordered::detail::func::call_destroy(
            constructor_.alloc_, p->value_ptr());
        boost::unordered::detail::func::destroy(boost::addressof(*p));
        node_allocator_traits::deallocate(constructor_.alloc_, p, 1);
    }
}

///////////////////////////////////////////////////////////////////
//
// Bucket

template <typename NodePointer> struct bucket
{
    typedef NodePointer link_pointer;
    link_pointer next_;

    bucket() : next_() {}

    link_pointer first_from_start() { return next_; }

    enum
    {
        extra_node = true
    };
};

struct ptr_bucket
{
    typedef ptr_bucket* link_pointer;
    link_pointer next_;

    ptr_bucket() : next_(0) {}

    link_pointer first_from_start() { return this; }

    enum
    {
        extra_node = false
    };
};

///////////////////////////////////////////////////////////////////
//
// Hash Policy

template <typename SizeT> struct prime_policy
{
    template <typename Hash, typename T>
    static inline SizeT apply_hash(Hash const& hf, T const& x)
    {
        return hf(x);
    }

    static inline SizeT to_bucket(SizeT bucket_count, SizeT hash)
    {
        return hash % bucket_count;
    }

    static inline SizeT new_bucket_count(SizeT min)
    {
        return boost::unordered::detail::next_prime(min);
    }

    static inline SizeT prev_bucket_count(SizeT max)
    {
        return boost::unordered::detail::prev_prime(max);
    }
};

template <typename SizeT> struct mix64_policy
{
    template <typename Hash, typename T>
    static inline SizeT apply_hash(Hash const& hf, T const& x)
    {
        SizeT key = hf(x);
        key = (~key) + (key << 21); // key = (key << 21) - key - 1;
        key = key ^ (key >> 24);
        key = (key + (key << 3)) + (key << 8); // key * 265
        key = key ^ (key >> 14);
        key = (key + (key << 2)) + (key << 4); // key * 21
        key = key ^ (key >> 28);
        key = key + (key << 31);
        return key;
    }

    static inline SizeT to_bucket(SizeT bucket_count, SizeT hash)
    {
        return hash & (bucket_count - 1);
    }

    static inline SizeT new_bucket_count(SizeT min)
    {
        if (min <= 4)
            return 4;
        --min;
        min |= min >> 1;
        min |= min >> 2;
        min |= min >> 4;
        min |= min >> 8;
        min |= min >> 16;
        min |= min >> 32;
        return min + 1;
    }

    static inline SizeT prev_bucket_count(SizeT max)
    {
        max |= max >> 1;
        max |= max >> 2;
        max |= max >> 4;
        max |= max >> 8;
        max |= max >> 16;
        max |= max >> 32;
        return (max >> 1) + 1;
    }
};

template <int digits, int radix> struct pick_policy_impl
{
    typedef prime_policy<std::size_t> type;
};

template <> struct pick_policy_impl<64, 2>
{
    typedef mix64_policy<std::size_t> type;
};

template <typename T>
struct pick_policy2 : pick_policy_impl<std::numeric_limits<std::size_t>::digits,
                          std::numeric_limits<std::size_t>::radix>
{
};

// While the mix policy is generally faster, the prime policy is a lot
// faster when a large number consecutive integers are used, because
// there are no collisions. Since that is probably quite common, use
// prime policy for integeral types. But not the smaller ones, as they
// don't have enough unique values for this to be an issue.

template <> struct pick_policy2<int>
{
    typedef prime_policy<std::size_t> type;
};

template <> struct pick_policy2<unsigned int>
{
    typedef prime_policy<std::size_t> type;
};

template <> struct pick_policy2<long>
{
    typedef prime_policy<std::size_t> type;
};

template <> struct pick_policy2<unsigned long>
{
    typedef prime_policy<std::size_t> type;
};

// TODO: Maybe not if std::size_t is smaller than long long.
#if !defined(BOOST_NO_LONG_LONG)
template <> struct pick_policy2<boost::long_long_type>
{
    typedef prime_policy<std::size_t> type;
};

template <> struct pick_policy2<boost::ulong_long_type>
{
    typedef prime_policy<std::size_t> type;
};
#endif

template <typename T>
struct pick_policy : pick_policy2<typename boost::remove_cv<T>::type>
{
};

////////////////////////////////////////////////////////////////////////////
// Functions

// Assigning and swapping the equality and hash function objects
// needs strong exception safety. To implement that normally we'd
// require one of them to be known to not throw and the other to
// guarantee strong exception safety. Unfortunately they both only
// have basic exception safety. So to acheive strong exception
// safety we have storage space for two copies, and assign the new
// copies to the unused space. Then switch to using that to use
// them. This is implemented in 'set_hash_functions' which
// atomically assigns the new function objects in a strongly
// exception safe manner.

template <class H, class P, bool NoThrowMoveAssign> class set_hash_functions;

template <class H, class P> class functions
{
  public:
    static const bool nothrow_move_assignable =
        boost::is_nothrow_move_assignable<H>::value &&
        boost::is_nothrow_move_assignable<P>::value;
    static const bool nothrow_move_constructible =
        boost::is_nothrow_move_constructible<H>::value &&
        boost::is_nothrow_move_constructible<P>::value;

  private:
    friend class boost::unordered::detail::set_hash_functions<H, P,
        nothrow_move_assignable>;
    functions& operator=(functions const&);

    typedef compressed<H, P> function_pair;

    typedef typename boost::aligned_storage<sizeof(function_pair),
        boost::alignment_of<function_pair>::value>::type aligned_function;

    bool current_; // The currently active functions.
    aligned_function funcs_[2];

    function_pair const& current() const
    {
        return *static_cast<function_pair const*>(
            static_cast<void const*>(&funcs_[current_]));
    }

    function_pair& current()
    {
        return *static_cast<function_pair*>(
            static_cast<void*>(&funcs_[current_]));
    }

    void construct(bool which, H const& hf, P const& eq)
    {
        new ((void*)&funcs_[which]) function_pair(hf, eq);
    }

    void construct(bool which, function_pair const& f,
        boost::unordered::detail::false_type =
            boost::unordered::detail::false_type())
    {
        new ((void*)&funcs_[which]) function_pair(f);
    }

    void construct(
        bool which, function_pair& f, boost::unordered::detail::true_type)
    {
        new ((void*)&funcs_[which])
            function_pair(f, boost::unordered::detail::move_tag());
    }

    void destroy(bool which)
    {
        boost::unordered::detail::func::destroy(
            (function_pair*)(&funcs_[which]));
    }

  public:
    typedef boost::unordered::detail::set_hash_functions<H, P,
        nothrow_move_assignable>
        set_hash_functions;

    functions(H const& hf, P const& eq) : current_(false)
    {
        construct(current_, hf, eq);
    }

    functions(functions const& bf) : current_(false)
    {
        construct(current_, bf.current());
    }

    functions(functions& bf, boost::unordered::detail::move_tag)
        : current_(false)
    {
        construct(current_, bf.current(),
            boost::unordered::detail::integral_constant<bool,
                      nothrow_move_constructible>());
    }

    ~functions() { this->destroy(current_); }

    H const& hash_function() const { return current().first(); }

    P const& key_eq() const { return current().second(); }
};

template <class H, class P> class set_hash_functions<H, P, false>
{
    set_hash_functions(set_hash_functions const&);
    set_hash_functions& operator=(set_hash_functions const&);

    typedef functions<H, P> functions_type;

    functions_type& functions_;
    bool tmp_functions_;

  public:
    set_hash_functions(functions_type& f, H const& h, P const& p)
        : functions_(f), tmp_functions_(!f.current_)
    {
        f.construct(tmp_functions_, h, p);
    }

    set_hash_functions(functions_type& f, functions_type const& other)
        : functions_(f), tmp_functions_(!f.current_)
    {
        f.construct(tmp_functions_, other.current());
    }

    ~set_hash_functions() { functions_.destroy(tmp_functions_); }

    void commit()
    {
        functions_.current_ = tmp_functions_;
        tmp_functions_ = !tmp_functions_;
    }
};

template <class H, class P> class set_hash_functions<H, P, true>
{
    set_hash_functions(set_hash_functions const&);
    set_hash_functions& operator=(set_hash_functions const&);

    typedef functions<H, P> functions_type;

    functions_type& functions_;
    H hash_;
    P pred_;

  public:
    set_hash_functions(functions_type& f, H const& h, P const& p)
        : functions_(f), hash_(h), pred_(p)
    {
    }

    set_hash_functions(functions_type& f, functions_type const& other)
        : functions_(f), hash_(other.hash_function()), pred_(other.key_eq())
    {
    }

    void commit()
    {
        functions_.current().first() = boost::move(hash_);
        functions_.current().second() = boost::move(pred_);
    }
};

////////////////////////////////////////////////////////////////////////////
// rvalue parameters when type can't be a BOOST_RV_REF(T) parameter
// e.g. for int

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#define BOOST_UNORDERED_RV_REF(T) BOOST_RV_REF(T)
#else
struct please_ignore_this_overload
{
    typedef please_ignore_this_overload type;
};

template <typename T> struct rv_ref_impl
{
    typedef BOOST_RV_REF(T) type;
};

template <typename T>
struct rv_ref
    : boost::detail::if_true<boost::is_class<T>::value>::BOOST_NESTED_TEMPLATE
          then<boost::unordered::detail::rv_ref_impl<T>,
              please_ignore_this_overload>::type
{
};

#define BOOST_UNORDERED_RV_REF(T)                                              \
    typename boost::unordered::detail::rv_ref<T>::type
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

////////////////////////////////////////////////////////////////////////////
// convert double to std::size_t

inline std::size_t double_to_size(double f)
{
    return f >= static_cast<double>((std::numeric_limits<std::size_t>::max)())
               ? (std::numeric_limits<std::size_t>::max)()
               : static_cast<std::size_t>(f);
}

// The space used to store values in a node.

template <typename ValueType> struct value_base
{
    typedef ValueType value_type;

    typename boost::aligned_storage<sizeof(value_type),
        boost::alignment_of<value_type>::value>::type data_;

    value_base() : data_() {}

    void* address() { return this; }

    value_type& value() { return *(ValueType*)this; }

    value_type const& value() const { return *(ValueType const*)this; }

    value_type* value_ptr() { return (ValueType*)this; }

    value_type const* value_ptr() const { return (ValueType const*)this; }

  private:
    value_base& operator=(value_base const&);
};

template <typename Types>
struct table : boost::unordered::detail::functions<typename Types::hasher,
                   typename Types::key_equal>
{
  private:
    table(table const&);
    table& operator=(table const&);

  public:
    typedef typename Types::node node;
    typedef typename Types::bucket bucket;
    typedef typename Types::hasher hasher;
    typedef typename Types::key_equal key_equal;
    typedef typename Types::const_key_type const_key_type;
    typedef typename Types::extractor extractor;
    typedef typename Types::value_type value_type;
    typedef typename Types::table table_impl;
    typedef typename Types::link_pointer link_pointer;
    typedef typename Types::policy policy;
    typedef typename Types::iterator iterator;
    typedef typename Types::c_iterator c_iterator;
    typedef typename Types::l_iterator l_iterator;
    typedef typename Types::cl_iterator cl_iterator;
    typedef typename Types::node_algo node_algo;

    typedef boost::unordered::detail::functions<typename Types::hasher,
        typename Types::key_equal>
        functions;
    typedef typename functions::set_hash_functions set_hash_functions;

    typedef typename Types::value_allocator value_allocator;
    typedef typename boost::unordered::detail::rebind_wrap<value_allocator,
        node>::type node_allocator;
    typedef typename boost::unordered::detail::rebind_wrap<value_allocator,
        bucket>::type bucket_allocator;
    typedef boost::unordered::detail::allocator_traits<node_allocator>
        node_allocator_traits;
    typedef boost::unordered::detail::allocator_traits<bucket_allocator>
        bucket_allocator_traits;
    typedef typename node_allocator_traits::pointer node_pointer;
    typedef typename node_allocator_traits::const_pointer const_node_pointer;
    typedef typename bucket_allocator_traits::pointer bucket_pointer;
    typedef boost::unordered::detail::node_constructor<node_allocator>
        node_constructor;
    typedef boost::unordered::detail::node_tmp<node_allocator> node_tmp;

    ////////////////////////////////////////////////////////////////////////
    // Members

    boost::unordered::detail::compressed<bucket_allocator, node_allocator>
        allocators_;
    std::size_t bucket_count_;
    std::size_t size_;
    float mlf_;
    std::size_t max_load_;
    bucket_pointer buckets_;

    ////////////////////////////////////////////////////////////////////////
    // Data access

    bucket_allocator const& bucket_alloc() const { return allocators_.first(); }

    node_allocator const& node_alloc() const { return allocators_.second(); }

    bucket_allocator& bucket_alloc() { return allocators_.first(); }

    node_allocator& node_alloc() { return allocators_.second(); }

    std::size_t max_bucket_count() const
    {
        // -1 to account for the start bucket.
        return policy::prev_bucket_count(
            bucket_allocator_traits::max_size(bucket_alloc()) - 1);
    }

    bucket_pointer get_bucket(std::size_t bucket_index) const
    {
        BOOST_ASSERT(buckets_);
        return buckets_ + static_cast<std::ptrdiff_t>(bucket_index);
    }

    link_pointer get_previous_start() const
    {
        return get_bucket(bucket_count_)->first_from_start();
    }

    link_pointer get_previous_start(std::size_t bucket_index) const
    {
        return get_bucket(bucket_index)->next_;
    }

    node_pointer begin() const
    {
        return size_ ? node_algo::next_node(get_previous_start())
                     : node_pointer();
    }

    node_pointer begin(std::size_t bucket_index) const
    {
        if (!size_)
            return node_pointer();
        link_pointer prev = get_previous_start(bucket_index);
        return prev ? node_algo::next_node(prev) : node_pointer();
    }

    std::size_t hash_to_bucket(std::size_t hash_value) const
    {
        return policy::to_bucket(bucket_count_, hash_value);
    }

    float load_factor() const
    {
        BOOST_ASSERT(bucket_count_ != 0);
        return static_cast<float>(size_) / static_cast<float>(bucket_count_);
    }

    std::size_t bucket_size(std::size_t index) const
    {
        node_pointer n = begin(index);
        if (!n)
            return 0;

        std::size_t count = 0;
        while (n && hash_to_bucket(n->hash_) == index) {
            ++count;
            n = node_algo::next_node(n);
        }

        return count;
    }

    ////////////////////////////////////////////////////////////////////////
    // Load methods

    std::size_t max_size() const
    {
        using namespace std;

        // size < mlf_ * count
        return boost::unordered::detail::double_to_size(
                   ceil(static_cast<double>(mlf_) *
                        static_cast<double>(max_bucket_count()))) -
               1;
    }

    void recalculate_max_load()
    {
        using namespace std;

        // From 6.3.1/13:
        // Only resize when size >= mlf_ * count
        max_load_ = buckets_ ? boost::unordered::detail::double_to_size(
                                   ceil(static_cast<double>(mlf_) *
                                        static_cast<double>(bucket_count_)))
                             : 0;
    }

    void max_load_factor(float z)
    {
        BOOST_ASSERT(z > 0);
        mlf_ = (std::max)(z, minimum_max_load_factor);
        recalculate_max_load();
    }

    std::size_t min_buckets_for_size(std::size_t size) const
    {
        BOOST_ASSERT(mlf_ >= minimum_max_load_factor);

        using namespace std;

        // From insert/emplace requirements:
        //
        // size <= mlf_ * count
        // => count >= size / mlf_
        //
        // Or from rehash post-condition:
        //
        // count >= size / mlf_

        return policy::new_bucket_count(
            boost::unordered::detail::double_to_size(
                floor(static_cast<double>(size) / static_cast<double>(mlf_)) +
                1));
    }

    ////////////////////////////////////////////////////////////////////////
    // Constructors

    table(std::size_t num_buckets, hasher const& hf, key_equal const& eq,
        node_allocator const& a)
        : functions(hf, eq), allocators_(a, a),
          bucket_count_(policy::new_bucket_count(num_buckets)), size_(0),
          mlf_(1.0f), max_load_(0), buckets_()
    {
    }

    table(table const& x, node_allocator const& a)
        : functions(x), allocators_(a, a),
          bucket_count_(x.min_buckets_for_size(x.size_)), size_(0),
          mlf_(x.mlf_), max_load_(0), buckets_()
    {
    }

    table(table& x, boost::unordered::detail::move_tag m)
        : functions(x, m), allocators_(x.allocators_, m),
          bucket_count_(x.bucket_count_), size_(x.size_), mlf_(x.mlf_),
          max_load_(x.max_load_), buckets_(x.buckets_)
    {
        x.buckets_ = bucket_pointer();
        x.size_ = 0;
        x.max_load_ = 0;
    }

    table(
        table& x, node_allocator const& a, boost::unordered::detail::move_tag m)
        : functions(x, m), allocators_(a, a), bucket_count_(x.bucket_count_),
          size_(0), mlf_(x.mlf_), max_load_(x.max_load_), buckets_()
    {
    }

    ////////////////////////////////////////////////////////////////////////
    // Initialisation.

    void init(table const& x)
    {
        if (x.size_) {
            static_cast<table_impl*>(this)->copy_buckets(x);
        }
    }

    void move_init(table& x)
    {
        if (node_alloc() == x.node_alloc()) {
            move_buckets_from(x);
        } else if (x.size_) {
            // TODO: Could pick new bucket size?
            static_cast<table_impl*>(this)->move_buckets(x);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Create buckets

    void create_buckets(std::size_t new_count)
    {
        std::size_t length = new_count + 1;
        bucket_pointer new_buckets =
            bucket_allocator_traits::allocate(bucket_alloc(), length);
        bucket_pointer constructed = new_buckets;

        BOOST_TRY
        {
            bucket_pointer end =
                new_buckets + static_cast<std::ptrdiff_t>(length);
            for (; constructed != end; ++constructed) {
                new ((void*)boost::addressof(*constructed)) bucket();
            }

            if (buckets_) {
                // Copy the nodes to the new buckets, including the dummy
                // node if there is one.
                (new_buckets + static_cast<std::ptrdiff_t>(new_count))->next_ =
                    (buckets_ + static_cast<std::ptrdiff_t>(bucket_count_))
                        ->next_;
                destroy_buckets();
            } else if (bucket::extra_node) {
                node_constructor a(node_alloc());
                a.create_node();

                (new_buckets + static_cast<std::ptrdiff_t>(new_count))->next_ =
                    a.release();
            }
        }
        BOOST_CATCH(...)
        {
            for (bucket_pointer p = new_buckets; p != constructed; ++p) {
                boost::unordered::detail::func::destroy(boost::addressof(*p));
            }

            bucket_allocator_traits::deallocate(
                bucket_alloc(), new_buckets, length);

            BOOST_RETHROW;
        }
        BOOST_CATCH_END

        bucket_count_ = new_count;
        buckets_ = new_buckets;
        recalculate_max_load();
    }

    ////////////////////////////////////////////////////////////////////////
    // Swap and Move

    void swap_allocators(table& other, false_type)
    {
        boost::unordered::detail::func::ignore_unused_variable_warning(other);

        // According to 23.2.1.8, if propagate_on_container_swap is
        // false the behaviour is undefined unless the allocators
        // are equal.
        BOOST_ASSERT(node_alloc() == other.node_alloc());
    }

    void swap_allocators(table& other, true_type)
    {
        allocators_.swap(other.allocators_);
    }

    // Only swaps the allocators if propagate_on_container_swap
    void swap(table& x)
    {
        set_hash_functions op1(*this, x);
        set_hash_functions op2(x, *this);

        // I think swap can throw if Propagate::value,
        // since the allocators' swap can throw. Not sure though.
        swap_allocators(x, boost::unordered::detail::integral_constant<bool,
                               allocator_traits<node_allocator>::
                                   propagate_on_container_swap::value>());

        boost::swap(buckets_, x.buckets_);
        boost::swap(bucket_count_, x.bucket_count_);
        boost::swap(size_, x.size_);
        std::swap(mlf_, x.mlf_);
        std::swap(max_load_, x.max_load_);
        op1.commit();
        op2.commit();
    }

    // Only call with nodes allocated with the currect allocator, or
    // one that is equal to it. (Can't assert because other's
    // allocators might have already been moved).
    void move_buckets_from(table& other)
    {
        BOOST_ASSERT(!buckets_);
        buckets_ = other.buckets_;
        bucket_count_ = other.bucket_count_;
        size_ = other.size_;
        other.buckets_ = bucket_pointer();
        other.size_ = 0;
        other.max_load_ = 0;
    }

    ////////////////////////////////////////////////////////////////////////
    // Delete/destruct

    ~table() { delete_buckets(); }

    void delete_node(link_pointer prev)
    {
        node_pointer n = static_cast<node_pointer>(prev->next_);
        prev->next_ = n->next_;

        boost::unordered::detail::func::call_destroy(
            node_alloc(), n->value_ptr());
        boost::unordered::detail::func::destroy(boost::addressof(*n));
        node_allocator_traits::deallocate(node_alloc(), n, 1);
        --size_;
    }

    std::size_t delete_nodes(link_pointer prev, link_pointer end)
    {
        BOOST_ASSERT(prev->next_ != end);

        std::size_t count = 0;

        do {
            delete_node(prev);
            ++count;
        } while (prev->next_ != end);

        return count;
    }

    void delete_buckets()
    {
        if (buckets_) {
            if (size_)
                delete_nodes(get_previous_start(), link_pointer());

            if (bucket::extra_node) {
                node_pointer n =
                    static_cast<node_pointer>(get_bucket(bucket_count_)->next_);
                boost::unordered::detail::func::destroy(boost::addressof(*n));
                node_allocator_traits::deallocate(node_alloc(), n, 1);
            }

            destroy_buckets();
            buckets_ = bucket_pointer();
            max_load_ = 0;
        }

        BOOST_ASSERT(!size_);
    }

    void clear()
    {
        if (!size_)
            return;

        delete_nodes(get_previous_start(), link_pointer());
        clear_buckets();

        BOOST_ASSERT(!size_);
    }

    void clear_buckets()
    {
        bucket_pointer end = get_bucket(bucket_count_);
        for (bucket_pointer it = buckets_; it != end; ++it) {
            it->next_ = node_pointer();
        }
    }

    void destroy_buckets()
    {
        bucket_pointer end = get_bucket(bucket_count_ + 1);
        for (bucket_pointer it = buckets_; it != end; ++it) {
            boost::unordered::detail::func::destroy(boost::addressof(*it));
        }

        bucket_allocator_traits::deallocate(
            bucket_alloc(), buckets_, bucket_count_ + 1);
    }

    ////////////////////////////////////////////////////////////////////////
    // Fix buckets after delete
    //

    std::size_t fix_bucket(std::size_t bucket_index, link_pointer prev)
    {
        link_pointer end = prev->next_;
        std::size_t bucket_index2 = bucket_index;

        if (end) {
            bucket_index2 =
                hash_to_bucket(static_cast<node_pointer>(end)->hash_);

            // If begin and end are in the same bucket, then
            // there's nothing to do.
            if (bucket_index == bucket_index2)
                return bucket_index2;

            // Update the bucket containing end.
            get_bucket(bucket_index2)->next_ = prev;
        }

        // Check if this bucket is now empty.
        bucket_pointer this_bucket = get_bucket(bucket_index);
        if (this_bucket->next_ == prev)
            this_bucket->next_ = link_pointer();

        return bucket_index2;
    }

    ////////////////////////////////////////////////////////////////////////
    // Assignment

    void assign(table const& x)
    {
        if (this != boost::addressof(x)) {
            assign(x, boost::unordered::detail::integral_constant<bool,
                          allocator_traits<node_allocator>::
                              propagate_on_container_copy_assignment::value>());
        }
    }

    void assign(table const& x, false_type)
    {
        // Strong exception safety.
        set_hash_functions new_func_this(*this, x);
        mlf_ = x.mlf_;
        recalculate_max_load();

        if (!size_ && !x.size_) {
            new_func_this.commit();
            return;
        }

        if (x.size_ >= max_load_) {
            create_buckets(min_buckets_for_size(x.size_));
        } else {
            clear_buckets();
        }

        new_func_this.commit();
        static_cast<table_impl*>(this)->assign_buckets(x);
    }

    void assign(table const& x, true_type)
    {
        if (node_alloc() == x.node_alloc()) {
            allocators_.assign(x.allocators_);
            assign(x, false_type());
        } else {
            set_hash_functions new_func_this(*this, x);

            // Delete everything with current allocators before assigning
            // the new ones.
            delete_buckets();
            allocators_.assign(x.allocators_);

            // Copy over other data, all no throw.
            new_func_this.commit();
            mlf_ = x.mlf_;
            bucket_count_ = min_buckets_for_size(x.size_);
            max_load_ = 0;

            // Finally copy the elements.
            if (x.size_) {
                static_cast<table_impl*>(this)->copy_buckets(x);
            }
        }
    }

    void move_assign(table& x)
    {
        if (this != boost::addressof(x)) {
            move_assign(
                x, boost::unordered::detail::integral_constant<bool,
                       allocator_traits<node_allocator>::
                           propagate_on_container_move_assignment::value>());
        }
    }

    void move_assign(table& x, true_type)
    {
        delete_buckets();
        set_hash_functions new_func_this(*this, x);
        allocators_.move_assign(x.allocators_);
        // No throw from here.
        mlf_ = x.mlf_;
        max_load_ = x.max_load_;
        move_buckets_from(x);
        new_func_this.commit();
    }

    void move_assign(table& x, false_type)
    {
        if (node_alloc() == x.node_alloc()) {
            delete_buckets();
            set_hash_functions new_func_this(*this, x);
            // No throw from here.
            mlf_ = x.mlf_;
            max_load_ = x.max_load_;
            move_buckets_from(x);
            new_func_this.commit();
        } else {
            set_hash_functions new_func_this(*this, x);
            mlf_ = x.mlf_;
            recalculate_max_load();

            if (!size_ && !x.size_) {
                new_func_this.commit();
                return;
            }

            if (x.size_ >= max_load_) {
                create_buckets(min_buckets_for_size(x.size_));
            } else {
                clear_buckets();
            }

            new_func_this.commit();
            static_cast<table_impl*>(this)->move_assign_buckets(x);
        }
    }

    // Accessors

    const_key_type& get_key(value_type const& x) const
    {
        return extractor::extract(x);
    }

    std::size_t hash(const_key_type& k) const
    {
        return policy::apply_hash(this->hash_function(), k);
    }

    // Find Node

    template <typename Key, typename Hash, typename Pred>
    node_pointer generic_find_node(
        Key const& k, Hash const& hf, Pred const& eq) const
    {
        return this->find_node_impl(policy::apply_hash(hf, k), k, eq);
    }

    node_pointer find_node(std::size_t key_hash, const_key_type& k) const
    {
        return this->find_node_impl(key_hash, k, this->key_eq());
    }

    node_pointer find_node(const_key_type& k) const
    {
        return this->find_node_impl(hash(k), k, this->key_eq());
    }

    template <class Key, class Pred>
    node_pointer find_node_impl(
        std::size_t key_hash, Key const& k, Pred const& eq) const
    {
        std::size_t bucket_index = this->hash_to_bucket(key_hash);
        node_pointer n = this->begin(bucket_index);

        for (;;) {
            if (!n)
                return n;

            std::size_t node_hash = n->hash_;
            if (key_hash == node_hash) {
                if (eq(k, this->get_key(n->value())))
                    return n;
            } else {
                if (this->hash_to_bucket(node_hash) != bucket_index)
                    return node_pointer();
            }

            n = node_algo::next_for_find(n);
        }
    }

    // Find the node before the key, so that it can be erased.
    link_pointer find_previous_node(
        const_key_type& k, std::size_t key_hash, std::size_t bucket_index)
    {
        link_pointer prev = this->get_previous_start(bucket_index);
        if (!prev) {
            return prev;
        }

        for (;;) {
            if (!prev->next_) {
                return link_pointer();
            }
            std::size_t node_hash = node_algo::next_node(prev)->hash_;
            if (this->hash_to_bucket(node_hash) != bucket_index) {
                return link_pointer();
            }
            if (node_hash == key_hash &&
                this->key_eq()(
                    k, this->get_key(node_algo::next_node(prev)->value()))) {
                return prev;
            }
            prev = node_algo::next_for_erase(prev);
        }
    }

    // Extract and erase

    inline node_pointer extract_by_key(const_key_type& k)
    {
        if (!this->size_) {
            return node_pointer();
        }
        std::size_t key_hash = this->hash(k);
        std::size_t bucket_index = this->hash_to_bucket(key_hash);
        link_pointer prev = this->find_previous_node(k, key_hash, bucket_index);
        if (!prev) {
            return node_pointer();
        }
        node_pointer n = node_algo::extract_first_node(prev);
        --this->size_;
        this->fix_bucket(bucket_index, prev);
        n->next_ = link_pointer();

        return n;
    }

    // Reserve and rehash

    void reserve_for_insert(std::size_t);
    void rehash(std::size_t);
    void reserve(std::size_t);
    void rehash_impl(std::size_t);
};

////////////////////////////////////////////////////////////////////////////
// Reserve & Rehash

// basic exception safety
template <typename Types>
inline void table<Types>::reserve_for_insert(std::size_t size)
{
    if (!buckets_) {
        create_buckets((std::max)(bucket_count_, min_buckets_for_size(size)));
    } else if (size > max_load_) {
        std::size_t num_buckets =
            min_buckets_for_size((std::max)(size, size_ + (size_ >> 1)));

        if (num_buckets != bucket_count_)
            this->rehash_impl(num_buckets);
    }
}

// if hash function throws, basic exception safety
// strong otherwise.

template <typename Types>
inline void table<Types>::rehash(std::size_t min_buckets)
{
    using namespace std;

    if (!size_) {
        delete_buckets();
        bucket_count_ = policy::new_bucket_count(min_buckets);
    } else {
        min_buckets = policy::new_bucket_count((std::max)(min_buckets,
            boost::unordered::detail::double_to_size(
                floor(static_cast<double>(size_) / static_cast<double>(mlf_))) +
                1));

        if (min_buckets != bucket_count_)
            this->rehash_impl(min_buckets);
    }
}

template <typename Types>
inline void table<Types>::reserve(std::size_t num_elements)
{
    rehash(static_cast<std::size_t>(
        std::ceil(static_cast<double>(num_elements) / mlf_)));
}

template <typename Types>
inline void table<Types>::rehash_impl(std::size_t num_buckets)
{
    BOOST_ASSERT(this->buckets_);

    this->create_buckets(num_buckets);
    link_pointer prev = this->get_previous_start();
    while (prev->next_) {
        node_pointer group_last = node_algo::last_for_rehash(prev);
        bucket_pointer b =
            this->get_bucket(this->hash_to_bucket(group_last->hash_));
        if (!b->next_) {
            b->next_ = prev;
            prev = group_last;
        } else {
            link_pointer next = group_last->next_;
            group_last->next_ = b->next_->next_;
            b->next_->next_ = prev->next_;
            prev->next_ = next;
        }
    }
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

////////////////////////////////////////////////////////////////////////
// key extractors
//
// no throw
//
// 'extract_key' is called with the emplace parameters to return a
// key if available or 'no_key' is one isn't and will need to be
// constructed. This could be done by overloading the emplace implementation
// for the different cases, but that's a bit tricky on compilers without
// variadic templates.

struct no_key
{
    no_key() {}
    template <class T> no_key(T const&) {}
};

template <typename Key, typename T> struct is_key
{
    template <typename T2> static choice1::type test(T2 const&);
    static choice2::type test(Key const&);

    enum
    {
        value = sizeof(test(boost::unordered::detail::make<T>())) ==
                sizeof(choice2::type)
    };

    typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE
        then<Key const&, no_key>::type type;
};

template <class ValueType> struct set_extractor
{
    typedef ValueType value_type;
    typedef ValueType key_type;

    static key_type const& extract(value_type const& v) { return v; }

    static no_key extract() { return no_key(); }

    template <class Arg> static no_key extract(Arg const&) { return no_key(); }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class Arg1, class Arg2, class... Args>
    static no_key extract(Arg1 const&, Arg2 const&, Args const&...)
    {
        return no_key();
    }
#else
    template <class Arg1, class Arg2>
    static no_key extract(Arg1 const&, Arg2 const&)
    {
        return no_key();
    }
#endif
};

template <class ValueType> struct map_extractor
{
    typedef ValueType value_type;
    typedef typename boost::remove_const<typename boost::unordered::detail::
            pair_traits<ValueType>::first_type>::type key_type;

    static key_type const& extract(value_type const& v) { return v.first; }

    template <class Second>
    static key_type const& extract(std::pair<key_type, Second> const& v)
    {
        return v.first;
    }

    template <class Second>
    static key_type const& extract(std::pair<key_type const, Second> const& v)
    {
        return v.first;
    }

    template <class Arg1>
    static key_type const& extract(key_type const& k, Arg1 const&)
    {
        return k;
    }

    static no_key extract() { return no_key(); }

    template <class Arg> static no_key extract(Arg const&) { return no_key(); }

    template <class Arg1, class Arg2>
    static no_key extract(Arg1 const&, Arg2 const&)
    {
        return no_key();
    }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <class Arg1, class Arg2, class Arg3, class... Args>
    static no_key extract(Arg1 const&, Arg2 const&, Arg3 const&, Args const&...)
    {
        return no_key();
    }
#endif

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_KEY_FROM_TUPLE(namespace_)                             \
    template <typename T2>                                                     \
    static no_key extract(boost::unordered::piecewise_construct_t,             \
        namespace_ tuple<> const&, T2 const&)                                  \
    {                                                                          \
        return no_key();                                                       \
    }                                                                          \
                                                                               \
    template <typename T, typename T2>                                         \
    static typename is_key<key_type, T>::type extract(                         \
        boost::unordered::piecewise_construct_t, namespace_ tuple<T> const& k, \
        T2 const&)                                                             \
    {                                                                          \
        return typename is_key<key_type, T>::type(namespace_ get<0>(k));       \
    }

#else

#define BOOST_UNORDERED_KEY_FROM_TUPLE(namespace_)                             \
    static no_key extract(                                                     \
        boost::unordered::piecewise_construct_t, namespace_ tuple<> const&)    \
    {                                                                          \
        return no_key();                                                       \
    }                                                                          \
                                                                               \
    template <typename T>                                                      \
    static typename is_key<key_type, T>::type extract(                         \
        boost::unordered::piecewise_construct_t, namespace_ tuple<T> const& k) \
    {                                                                          \
        return typename is_key<key_type, T>::type(namespace_ get<0>(k));       \
    }

#endif

    BOOST_UNORDERED_KEY_FROM_TUPLE(boost::)

#if !defined(BOOST_NO_CXX11_HDR_TUPLE)
    BOOST_UNORDERED_KEY_FROM_TUPLE(std::)
#endif
};

////////////////////////////////////////////////////////////////////////
// Unique nodes

template <typename A, typename T>
struct unique_node : boost::unordered::detail::value_base<T>
{
    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        unique_node<A, T> >::type allocator;
    typedef typename ::boost::unordered::detail::allocator_traits<
        allocator>::pointer node_pointer;
    typedef node_pointer link_pointer;
    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        bucket<node_pointer> >::type bucket_allocator;
    typedef typename ::boost::unordered::detail::allocator_traits<
        bucket_allocator>::pointer bucket_pointer;

    link_pointer next_;
    std::size_t hash_;

    unique_node() : next_(), hash_(0) {}

    void init(node_pointer) {}

  private:
    unique_node& operator=(unique_node const&);
};

template <typename T> struct ptr_node : boost::unordered::detail::ptr_bucket
{
    typedef T value_type;
    typedef boost::unordered::detail::ptr_bucket bucket_base;
    typedef ptr_node<T>* node_pointer;
    typedef ptr_bucket* link_pointer;
    typedef ptr_bucket* bucket_pointer;

    std::size_t hash_;
    boost::unordered::detail::value_base<T> value_base_;

    ptr_node() : bucket_base(), hash_(0) {}

    void init(node_pointer) {}

    void* address() { return value_base_.address(); }
    value_type& value() { return value_base_.value(); }
    value_type* value_ptr() { return value_base_.value_ptr(); }

  private:
    ptr_node& operator=(ptr_node const&);
};

template <typename N> struct node_algo
{
    typedef typename N::node_pointer node_pointer;
    typedef typename N::link_pointer link_pointer;
    typedef typename N::bucket_pointer bucket_pointer;

    static node_pointer next_node(link_pointer n)
    {
        return static_cast<node_pointer>(n->next_);
    }

    static node_pointer next_for_find(node_pointer n)
    {
        return static_cast<node_pointer>(n->next_);
    }

    static link_pointer next_for_erase(link_pointer prev)
    {
        return prev->next_;
    }

    // Group together all nodes with equal hash value, this may
    // include nodes with different keys, but that's okay because
    // they will end up in the same bucket.
    static node_pointer last_for_rehash(link_pointer prev)
    {
        node_pointer n = next_node(prev);
        std::size_t hash = n->hash_;
        for (;;) {
            node_pointer next = next_node(n);
            if (!next || next->hash_ != hash) {
                return n;
            }
            n = next;
        }
    }

    template <typename Table>
    static node_pointer next_group(node_pointer n, Table const* t)
    {
        node_pointer n1 = n;
        do {
            n1 = next_node(n1);
        } while (
            n1 && t->key_eq()(t->get_key(n->value()), t->get_key(n1->value())));
        return n1;
    }

    template <typename Table>
    static std::size_t count(node_pointer n, Table const* t)
    {
        std::size_t x = 0;
        node_pointer it = n;
        do {
            ++x;
            it = next_node(it);
        } while (
            it && t->key_eq()(t->get_key(n->value()), t->get_key(it->value())));

        return x;
    }

    // Add node 'n' after 'pos'.
    // This results in a different order to the grouped implementation.
    static inline void add_to_node_group(node_pointer n, node_pointer pos)
    {
        n->next_ = pos->next_;
        pos->next_ = n;
    }

    static inline node_pointer extract_first_node(link_pointer prev)
    {
        node_pointer n = next_node(prev);
        prev->next_ = n->next_;
        return n;
    }

    static link_pointer split_groups(node_pointer, node_pointer)
    {
        return link_pointer();
    }
};

// If the allocator uses raw pointers use ptr_node
// Otherwise use node.

template <typename A, typename T, typename NodePtr, typename BucketPtr>
struct pick_node2
{
    typedef boost::unordered::detail::unique_node<A, T> node;

    typedef typename boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A, node>::type>::pointer
        node_pointer;

    typedef boost::unordered::detail::bucket<node_pointer> bucket;
    typedef node_pointer link_pointer;
};

template <typename A, typename T>
struct pick_node2<A, T, boost::unordered::detail::ptr_node<T>*,
    boost::unordered::detail::ptr_bucket*>
{
    typedef boost::unordered::detail::ptr_node<T> node;
    typedef boost::unordered::detail::ptr_bucket bucket;
    typedef bucket* link_pointer;
};

template <typename A, typename T> struct pick_node
{
    typedef typename boost::remove_const<T>::type nonconst;

    typedef boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::ptr_node<nonconst> >::type>
        tentative_node_traits;

    typedef boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::ptr_bucket>::type>
        tentative_bucket_traits;

    typedef pick_node2<A, nonconst, typename tentative_node_traits::pointer,
        typename tentative_bucket_traits::pointer>
        pick;

    typedef typename pick::node node;
    typedef typename pick::bucket bucket;
    typedef typename pick::link_pointer link_pointer;
    typedef boost::unordered::detail::node_algo<node> node_algo;
};

template <typename Types>
struct table_impl : boost::unordered::detail::table<Types>
{
    typedef boost::unordered::detail::table<Types> table;
    typedef typename table::value_type value_type;
    typedef typename table::node node;
    typedef typename table::bucket bucket;
    typedef typename table::policy policy;
    typedef typename table::node_pointer node_pointer;
    typedef typename table::node_allocator node_allocator;
    typedef typename table::node_allocator_traits node_allocator_traits;
    typedef typename table::bucket_pointer bucket_pointer;
    typedef typename table::link_pointer link_pointer;
    typedef typename table::hasher hasher;
    typedef typename table::key_equal key_equal;
    typedef typename table::const_key_type const_key_type;
    typedef typename table::node_constructor node_constructor;
    typedef typename table::node_tmp node_tmp;
    typedef typename table::extractor extractor;
    typedef typename table::iterator iterator;
    typedef typename table::c_iterator c_iterator;
    typedef typename table::node_algo node_algo;

    typedef std::pair<iterator, bool> emplace_return;

    // Constructors

    table_impl(std::size_t n, hasher const& hf, key_equal const& eq,
        node_allocator const& a)
        : table(n, hf, eq, a)
    {
    }

    table_impl(table_impl const& x)
        : table(x, node_allocator_traits::select_on_container_copy_construction(
                       x.node_alloc()))
    {
        this->init(x);
    }

    table_impl(table_impl const& x, node_allocator const& a) : table(x, a)
    {
        this->init(x);
    }

    table_impl(table_impl& x, boost::unordered::detail::move_tag m)
        : table(x, m)
    {
    }

    table_impl(table_impl& x, node_allocator const& a,
        boost::unordered::detail::move_tag m)
        : table(x, a, m)
    {
        this->move_init(x);
    }

    // Accessors

    std::size_t count(const_key_type& k) const
    {
        return this->find_node(k) ? 1 : 0;
    }

    value_type& at(const_key_type& k) const
    {
        if (this->size_) {
            node_pointer n = this->find_node(k);
            if (n)
                return n->value();
        }

        boost::throw_exception(
            std::out_of_range("Unable to find key in unordered_map."));
    }

    std::pair<iterator, iterator> equal_range(const_key_type& k) const
    {
        node_pointer n = this->find_node(k);
        return std::make_pair(
            iterator(n), iterator(n ? node_algo::next_node(n) : n));
    }

    // equals

    bool equals(table_impl const& other) const
    {
        if (this->size_ != other.size_)
            return false;

        for (node_pointer n1 = this->begin(); n1;
             n1 = node_algo::next_node(n1)) {
            node_pointer n2 = other.find_node(other.get_key(n1->value()));

            if (!n2 || n1->value() != n2->value())
                return false;
        }

        return true;
    }

    // Emplace/Insert

    inline node_pointer add_node(node_pointer n, std::size_t key_hash)
    {
        n->hash_ = key_hash;

        bucket_pointer b = this->get_bucket(this->hash_to_bucket(key_hash));

        if (!b->next_) {
            link_pointer start_node = this->get_previous_start();

            if (start_node->next_) {
                this->get_bucket(this->hash_to_bucket(
                                     node_algo::next_node(start_node)->hash_))
                    ->next_ = n;
            }

            b->next_ = start_node;
            n->next_ = start_node->next_;
            start_node->next_ = n;
        } else {
            n->next_ = b->next_->next_;
            b->next_->next_ = n;
        }

        ++this->size_;
        return n;
    }

    inline node_pointer resize_and_add_node(
        node_pointer n, std::size_t key_hash)
    {
        node_tmp b(n, this->node_alloc());
        this->reserve_for_insert(this->size_ + 1);
        return this->add_node(b.release(), key_hash);
    }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    emplace_return emplace(boost::unordered::detail::emplace_args1<
        boost::unordered::detail::please_ignore_this_overload> const&)
    {
        BOOST_ASSERT(false);
        return emplace_return(iterator(), false);
    }

    iterator emplace_hint(
        c_iterator,
        boost::unordered::detail::emplace_args1<
            boost::unordered::detail::please_ignore_this_overload> const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }
#else
    emplace_return emplace(
        boost::unordered::detail::please_ignore_this_overload const&)
    {
        BOOST_ASSERT(false);
        return emplace_return(iterator(), false);
    }

    iterator emplace_hint(c_iterator,
        boost::unordered::detail::please_ignore_this_overload const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }
#endif
#endif

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    emplace_return emplace(BOOST_UNORDERED_EMPLACE_ARGS)
    {
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        return emplace_impl(extractor::extract(BOOST_UNORDERED_EMPLACE_FORWARD),
            BOOST_UNORDERED_EMPLACE_FORWARD);
#else
        return emplace_impl(extractor::extract(args.a0, args.a1),
            BOOST_UNORDERED_EMPLACE_FORWARD);
#endif
    }

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator emplace_hint(c_iterator hint, BOOST_UNORDERED_EMPLACE_ARGS)
    {
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        return emplace_hint_impl(hint,
            extractor::extract(BOOST_UNORDERED_EMPLACE_FORWARD),
            BOOST_UNORDERED_EMPLACE_FORWARD);
#else
        return emplace_hint_impl(hint, extractor::extract(args.a0, args.a1),
            BOOST_UNORDERED_EMPLACE_FORWARD);
#endif
    }

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <typename A0>
    emplace_return emplace(
        boost::unordered::detail::emplace_args1<A0> const& args)
    {
        return emplace_impl(extractor::extract(args.a0), args);
    }

    template <typename A0>
    iterator emplace_hint(c_iterator hint,
        boost::unordered::detail::emplace_args1<A0> const& args)
    {
        return emplace_hint_impl(hint, extractor::extract(args.a0), args);
    }
#endif

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator emplace_hint_impl(
        c_iterator hint, const_key_type& k, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        if (hint.node_ && this->key_eq()(k, this->get_key(*hint))) {
            return iterator(hint.node_);
        } else {
            return emplace_impl(k, BOOST_UNORDERED_EMPLACE_FORWARD).first;
        }
    }

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    emplace_return emplace_impl(const_key_type& k, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (pos) {
            return emplace_return(iterator(pos), false);
        } else {
            return emplace_return(
                iterator(this->resize_and_add_node(
                    boost::unordered::detail::func::construct_node_from_args(
                        this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
                    key_hash)),
                true);
        }
    }

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator emplace_hint_impl(
        c_iterator hint, no_key, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        node_tmp b(boost::unordered::detail::func::construct_node_from_args(
                       this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
            this->node_alloc());
        const_key_type& k = this->get_key(b.node_->value());
        if (hint.node_ && this->key_eq()(k, this->get_key(*hint))) {
            return iterator(hint.node_);
        }
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (pos) {
            return iterator(pos);
        } else {
            return iterator(this->resize_and_add_node(b.release(), key_hash));
        }
    }

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    emplace_return emplace_impl(no_key, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        node_tmp b(boost::unordered::detail::func::construct_node_from_args(
                       this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
            this->node_alloc());
        const_key_type& k = this->get_key(b.node_->value());
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (pos) {
            return emplace_return(iterator(pos), false);
        } else {
            return emplace_return(
                iterator(this->resize_and_add_node(b.release(), key_hash)),
                true);
        }
    }

    template <typename Key>
    emplace_return try_emplace_impl(BOOST_FWD_REF(Key) k)
    {
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (pos) {
            return emplace_return(iterator(pos), false);
        } else {
            return emplace_return(
                iterator(this->resize_and_add_node(
                    boost::unordered::detail::func::construct_node_pair(
                        this->node_alloc(), boost::forward<Key>(k)),
                    key_hash)),
                true);
        }
    }

    template <typename Key>
    iterator try_emplace_hint_impl(c_iterator hint, BOOST_FWD_REF(Key) k)
    {
        if (hint.node_ && this->key_eq()(hint->first, k)) {
            return iterator(hint.node_);
        } else {
            return try_emplace_impl(k).first;
        }
    }

    template <typename Key, BOOST_UNORDERED_EMPLACE_TEMPLATE>
    emplace_return try_emplace_impl(
        BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (pos) {
            return emplace_return(iterator(pos), false);
        } else {
            return emplace_return(
                iterator(this->resize_and_add_node(
                    boost::unordered::detail::func::
                        construct_node_pair_from_args(this->node_alloc(),
                            boost::forward<Key>(k),
                            BOOST_UNORDERED_EMPLACE_FORWARD),
                    key_hash)),
                true);
        }
    }

    template <typename Key, BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator try_emplace_hint_impl(
        c_iterator hint, BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        if (hint.node_ && this->key_eq()(hint->first, k)) {
            return iterator(hint.node_);
        } else {
            return try_emplace_impl(k, BOOST_UNORDERED_EMPLACE_FORWARD).first;
        }
    }

    template <typename Key, typename M>
    emplace_return insert_or_assign_impl(
        BOOST_FWD_REF(Key) k, BOOST_FWD_REF(M) obj)
    {
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);

        if (pos) {
            pos->value().second = boost::forward<M>(obj);
            return emplace_return(iterator(pos), false);
        } else {
            return emplace_return(
                iterator(this->resize_and_add_node(
                    boost::unordered::detail::func::construct_node_pair(
                        this->node_alloc(), boost::forward<Key>(k),
                        boost::forward<M>(obj)),
                    key_hash)),
                true);
        }
    }

    template <typename NodeType, typename InsertReturnType>
    void move_insert_node_type(NodeType& np, InsertReturnType& result)
    {
        if (np) {
            const_key_type& k = this->get_key(np.ptr_->value());
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);

            if (pos) {
                result.node = boost::move(np);
                result.position = iterator(pos);
            } else {
                this->reserve_for_insert(this->size_ + 1);
                result.position = iterator(this->add_node(np.ptr_, key_hash));
                result.inserted = true;
                np.ptr_ = node_pointer();
            }
        }
    }

    template <typename NodeType>
    iterator move_insert_node_type_with_hint(c_iterator hint, NodeType& np)
    {
        if (!np) {
            return iterator();
        }
        const_key_type& k = this->get_key(np.ptr_->value());
        if (hint.node_ && this->key_eq()(k, this->get_key(*hint))) {
            return iterator(hint.node_);
        }
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);
        if (!pos) {
            this->reserve_for_insert(this->size_ + 1);
            pos = this->add_node(np.ptr_, key_hash);
            np.ptr_ = node_pointer();
        }
        return iterator(pos);
    }

    template <typename Types2>
    void merge_impl(boost::unordered::detail::table<Types2>& other)
    {
        typedef boost::unordered::detail::table<Types2> other_table;
        BOOST_STATIC_ASSERT(
            (boost::is_same<node, typename other_table::node>::value));
        BOOST_ASSERT(this->node_alloc() == other.node_alloc());

        if (other.size_) {
            link_pointer prev = other.get_previous_start();

            while (prev->next_) {
                node_pointer n = other_table::node_algo::next_node(prev);
                const_key_type& k = this->get_key(n->value());
                std::size_t key_hash = this->hash(k);
                node_pointer pos = this->find_node(key_hash, k);

                if (pos) {
                    prev = n;
                } else {
                    this->reserve_for_insert(this->size_ + 1);
                    other_table::node_algo::split_groups(
                        n, other_table::node_algo::next_node(n));
                    prev->next_ = n->next_;
                    --other.size_;
                    other.fix_bucket(other.hash_to_bucket(n->hash_), prev);
                    this->add_node(n, key_hash);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Insert range methods
    //
    // if hash function throws, or inserting > 1 element, basic exception
    // safety strong otherwise

    template <class InputIt> void insert_range(InputIt i, InputIt j)
    {
        if (i != j)
            return insert_range_impl(extractor::extract(*i), i, j);
    }

    template <class InputIt>
    void insert_range_impl(const_key_type& k, InputIt i, InputIt j)
    {
        insert_range_impl2(k, i, j);

        while (++i != j) {
            // Note: can't use get_key as '*i' might not be value_type - it
            // could be a pair with first_types as key_type without const or
            // a different second_type.
            //
            // TODO: Might be worth storing the value_type instead of the
            // key here. Could be more efficient if '*i' is expensive. Could
            // be less efficient if copying the full value_type is
            // expensive.
            insert_range_impl2(extractor::extract(*i), i, j);
        }
    }

    template <class InputIt>
    void insert_range_impl2(const_key_type& k, InputIt i, InputIt j)
    {
        // No side effects in this initial code
        std::size_t key_hash = this->hash(k);
        node_pointer pos = this->find_node(key_hash, k);

        if (!pos) {
            node_tmp b(boost::unordered::detail::func::construct_node(
                           this->node_alloc(), *i),
                this->node_alloc());
            if (this->size_ + 1 > this->max_load_)
                this->reserve_for_insert(
                    this->size_ + boost::unordered::detail::insert_size(i, j));
            this->add_node(b.release(), key_hash);
        }
    }

    template <class InputIt>
    void insert_range_impl(no_key, InputIt i, InputIt j)
    {
        node_constructor a(this->node_alloc());

        do {
            if (!a.node_) {
                a.create_node();
            }
            boost::unordered::detail::func::call_construct(
                a.alloc_, a.node_->value_ptr(), *i);
            node_tmp b(a.release(), a.alloc_);

            const_key_type& k = this->get_key(b.node_->value());
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);

            if (pos) {
                a.reclaim(b.release());
            } else {
                // reserve has basic exception safety if the hash function
                // throws, strong otherwise.
                this->reserve_for_insert(this->size_ + 1);
                this->add_node(b.release(), key_hash);
            }
        } while (++i != j);
    }

    ////////////////////////////////////////////////////////////////////////
    // Extract

    inline node_pointer extract_by_iterator(c_iterator i)
    {
        node_pointer n = i.node_;
        BOOST_ASSERT(n);
        std::size_t key_hash = n->hash_;
        std::size_t bucket_index = this->hash_to_bucket(key_hash);
        link_pointer prev = this->get_previous_start(bucket_index);
        while (prev->next_ != n) {
            prev = prev->next_;
        }
        prev->next_ = n->next_;
        --this->size_;
        this->fix_bucket(bucket_index, prev);
        n->next_ = link_pointer();
        return n;
    }

    ////////////////////////////////////////////////////////////////////////
    // Erase
    //
    // no throw

    std::size_t erase_key(const_key_type& k)
    {
        if (!this->size_)
            return 0;
        std::size_t key_hash = this->hash(k);
        std::size_t bucket_index = this->hash_to_bucket(key_hash);
        link_pointer prev = this->find_previous_node(k, key_hash, bucket_index);
        if (!prev)
            return 0;
        link_pointer end = node_algo::next_node(prev)->next_;
        this->delete_nodes(prev, end);
        this->fix_bucket(bucket_index, prev);
        return 1;
    }

    iterator erase(c_iterator r)
    {
        BOOST_ASSERT(r.node_);
        node_pointer next = node_algo::next_node(r.node_);
        erase_nodes(r.node_, next);
        return iterator(next);
    }

    iterator erase_range(c_iterator r1, c_iterator r2)
    {
        if (r1 == r2)
            return iterator(r2.node_);
        erase_nodes(r1.node_, r2.node_);
        return iterator(r2.node_);
    }

    void erase_nodes(node_pointer i, node_pointer j)
    {
        std::size_t bucket_index = this->hash_to_bucket(i->hash_);

        // Find the node before i.
        link_pointer prev = this->get_previous_start(bucket_index);
        while (prev->next_ != i)
            prev = prev->next_;

        // Delete the nodes.
        do {
            this->delete_node(prev);
            bucket_index = this->fix_bucket(bucket_index, prev);
        } while (prev->next_ != j);
    }

    ////////////////////////////////////////////////////////////////////////
    // fill_buckets

    void copy_buckets(table const& src)
    {
        this->create_buckets(this->bucket_count_);

        for (node_pointer n = src.begin(); n; n = node_algo::next_node(n)) {
            this->add_node(boost::unordered::detail::func::construct_node(
                               this->node_alloc(), n->value()),
                n->hash_);
        }
    }

    void move_buckets(table const& src)
    {
        this->create_buckets(this->bucket_count_);

        for (node_pointer n = src.begin(); n; n = node_algo::next_node(n)) {
            this->add_node(boost::unordered::detail::func::construct_node(
                               this->node_alloc(), boost::move(n->value())),
                n->hash_);
        }
    }

    void assign_buckets(table const& src)
    {
        node_holder<node_allocator> holder(*this);
        for (node_pointer n = src.begin(); n; n = node_algo::next_node(n)) {
            this->add_node(holder.copy_of(n->value()), n->hash_);
        }
    }

    void move_assign_buckets(table& src)
    {
        node_holder<node_allocator> holder(*this);
        for (node_pointer n = src.begin(); n; n = node_algo::next_node(n)) {
            this->add_node(holder.move_copy_of(n->value()), n->hash_);
        }
    }
};

////////////////////////////////////////////////////////////////////////
// Grouped nodes

template <typename A, typename T>
struct grouped_node : boost::unordered::detail::value_base<T>
{
    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        grouped_node<A, T> >::type allocator;
    typedef typename ::boost::unordered::detail::allocator_traits<
        allocator>::pointer node_pointer;
    typedef node_pointer link_pointer;
    typedef typename ::boost::unordered::detail::rebind_wrap<A,
        bucket<node_pointer> >::type bucket_allocator;
    typedef typename ::boost::unordered::detail::allocator_traits<
        bucket_allocator>::pointer bucket_pointer;

    link_pointer next_;
    node_pointer group_prev_;
    std::size_t hash_;

    grouped_node() : next_(), group_prev_(), hash_(0) {}

    void init(node_pointer self) { group_prev_ = self; }

  private:
    grouped_node& operator=(grouped_node const&);
};

template <typename T>
struct grouped_ptr_node : boost::unordered::detail::ptr_bucket
{
    typedef T value_type;
    typedef boost::unordered::detail::ptr_bucket bucket_base;
    typedef grouped_ptr_node<T>* node_pointer;
    typedef ptr_bucket* link_pointer;
    typedef ptr_bucket* bucket_pointer;

    node_pointer group_prev_;
    std::size_t hash_;
    boost::unordered::detail::value_base<T> value_base_;

    grouped_ptr_node() : bucket_base(), group_prev_(0), hash_(0) {}

    void init(node_pointer self) { group_prev_ = self; }

    void* address() { return value_base_.address(); }
    value_type& value() { return value_base_.value(); }
    value_type* value_ptr() { return value_base_.value_ptr(); }

  private:
    grouped_ptr_node& operator=(grouped_ptr_node const&);
};

template <typename N> struct grouped_node_algo
{
    typedef typename N::node_pointer node_pointer;
    typedef typename N::link_pointer link_pointer;
    typedef typename N::bucket_pointer bucket_pointer;

    static node_pointer next_node(link_pointer n)
    {
        return static_cast<node_pointer>(n->next_);
    }

    static node_pointer next_for_find(node_pointer n)
    {
        return static_cast<node_pointer>(n->group_prev_->next_);
    }

    static link_pointer next_for_erase(link_pointer prev)
    {
        return static_cast<node_pointer>(prev->next_)->group_prev_;
    }

    static node_pointer last_for_rehash(link_pointer prev)
    {
        return static_cast<node_pointer>(prev->next_)->group_prev_;
    }

    // The 'void*' arguments are pointers to the table, which we
    // will ignore, but without groups they could be used to
    // access the various functions for dealing with values and keys.
    static node_pointer next_group(node_pointer n, void const*)
    {
        return static_cast<node_pointer>(n->group_prev_->next_);
    }

    static std::size_t count(node_pointer n, void const*)
    {
        std::size_t x = 0;
        node_pointer it = n;
        do {
            it = it->group_prev_;
            ++x;
        } while (it != n);

        return x;
    }

    // Adds node 'n' to the group containing 'pos'.
    // If 'pos' is the first node in group, add to the end of the group,
    // otherwise add before 'pos'. Other versions will probably behave
    // differently.
    static inline void add_to_node_group(node_pointer n, node_pointer pos)
    {
        n->next_ = pos->group_prev_->next_;
        n->group_prev_ = pos->group_prev_;
        pos->group_prev_->next_ = n;
        pos->group_prev_ = n;
    }

    static inline node_pointer extract_first_node(link_pointer prev)
    {
        node_pointer n = next_node(prev);
        if (n->group_prev_ != n) {
            node_pointer next = next_node(n);
            next->group_prev_ = n->group_prev_;
            n->group_prev_ = n;
        }
        prev->next_ = n->next_;
        return n;
    }

    // Split the groups containing 'i' and 'j' so that they can
    // be safely erased/extracted.
    static link_pointer split_groups(node_pointer i, node_pointer j)
    {
        node_pointer prev = i->group_prev_;
        if (prev->next_ != i)
            prev = node_pointer();

        if (j) {
            node_pointer first = j;
            while (first != i && first->group_prev_->next_ == first) {
                first = first->group_prev_;
            }

            boost::swap(first->group_prev_, j->group_prev_);
            if (first == i)
                return prev;
        }

        if (prev) {
            node_pointer first = prev;
            while (first->group_prev_->next_ == first) {
                first = first->group_prev_;
            }
            boost::swap(first->group_prev_, i->group_prev_);
        }

        return prev;
    }
};

// If the allocator uses raw pointers use grouped_ptr_node
// Otherwise use grouped_node.

template <typename A, typename T, typename NodePtr, typename BucketPtr>
struct pick_grouped_node2
{
    typedef boost::unordered::detail::grouped_node<A, T> node;

    typedef typename boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A, node>::type>::pointer
        node_pointer;

    typedef boost::unordered::detail::bucket<node_pointer> bucket;
    typedef node_pointer link_pointer;
};

template <typename A, typename T>
struct pick_grouped_node2<A, T, boost::unordered::detail::grouped_ptr_node<T>*,
    boost::unordered::detail::ptr_bucket*>
{
    typedef boost::unordered::detail::grouped_ptr_node<T> node;
    typedef boost::unordered::detail::ptr_bucket bucket;
    typedef bucket* link_pointer;
};

template <typename A, typename T> struct pick_grouped_node
{
    typedef typename boost::remove_const<T>::type nonconst;

    typedef boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::grouped_ptr_node<nonconst> >::type>
        tentative_node_traits;

    typedef boost::unordered::detail::allocator_traits<
        typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::ptr_bucket>::type>
        tentative_bucket_traits;

    typedef pick_grouped_node2<A, nonconst,
        typename tentative_node_traits::pointer,
        typename tentative_bucket_traits::pointer>
        pick;

    typedef typename pick::node node;
    typedef typename pick::bucket bucket;
    typedef typename pick::link_pointer link_pointer;
    typedef boost::unordered::detail::grouped_node_algo<node> node_algo;
};

template <typename Types>
struct grouped_table_impl : boost::unordered::detail::table<Types>
{
    typedef boost::unordered::detail::table<Types> table;
    typedef typename table::value_type value_type;
    typedef typename table::bucket bucket;
    typedef typename table::policy policy;
    typedef typename table::node_pointer node_pointer;
    typedef typename table::node_allocator node_allocator;
    typedef typename table::node_allocator_traits node_allocator_traits;
    typedef typename table::bucket_pointer bucket_pointer;
    typedef typename table::link_pointer link_pointer;
    typedef typename table::hasher hasher;
    typedef typename table::key_equal key_equal;
    typedef typename table::const_key_type const_key_type;
    typedef typename table::node_constructor node_constructor;
    typedef typename table::node_tmp node_tmp;
    typedef typename table::extractor extractor;
    typedef typename table::iterator iterator;
    typedef typename table::c_iterator c_iterator;
    typedef typename table::node_algo node_algo;

    // Constructors

    grouped_table_impl(std::size_t n, hasher const& hf, key_equal const& eq,
        node_allocator const& a)
        : table(n, hf, eq, a)
    {
    }

    grouped_table_impl(grouped_table_impl const& x)
        : table(x, node_allocator_traits::select_on_container_copy_construction(
                       x.node_alloc()))
    {
        this->init(x);
    }

    grouped_table_impl(grouped_table_impl const& x, node_allocator const& a)
        : table(x, a)
    {
        this->init(x);
    }

    grouped_table_impl(
        grouped_table_impl& x, boost::unordered::detail::move_tag m)
        : table(x, m)
    {
    }

    grouped_table_impl(grouped_table_impl& x, node_allocator const& a,
        boost::unordered::detail::move_tag m)
        : table(x, a, m)
    {
        this->move_init(x);
    }

    // Accessors

    std::size_t count(const_key_type& k) const
    {
        node_pointer n = this->find_node(k);
        return n ? node_algo::count(n, this) : 0;
    }

    std::pair<iterator, iterator> equal_range(const_key_type& k) const
    {
        node_pointer n = this->find_node(k);
        return std::make_pair(
            iterator(n), iterator(n ? node_algo::next_group(n, this) : n));
    }

    // Equality

    bool equals(grouped_table_impl const& other) const
    {
        if (this->size_ != other.size_)
            return false;

        for (node_pointer n1 = this->begin(); n1;) {
            node_pointer n2 = other.find_node(other.get_key(n1->value()));
            if (!n2)
                return false;
            node_pointer end1 = node_algo::next_group(n1, this);
            node_pointer end2 = node_algo::next_group(n2, this);
            if (!group_equals(n1, end1, n2, end2))
                return false;
            n1 = end1;
        }

        return true;
    }

    static bool group_equals(
        node_pointer n1, node_pointer end1, node_pointer n2, node_pointer end2)
    {
        for (;;) {
            if (n1->value() != n2->value())
                break;

            n1 = node_algo::next_node(n1);
            n2 = node_algo::next_node(n2);

            if (n1 == end1)
                return n2 == end2;
            if (n2 == end2)
                return false;
        }

        for (node_pointer n1a = n1, n2a = n2;;) {
            n1a = node_algo::next_node(n1a);
            n2a = node_algo::next_node(n2a);

            if (n1a == end1) {
                if (n2a == end2)
                    break;
                else
                    return false;
            }

            if (n2a == end2)
                return false;
        }

        node_pointer start = n1;
        for (; n1 != end1; n1 = node_algo::next_node(n1)) {
            value_type const& v = n1->value();
            if (!find(start, n1, v)) {
                std::size_t matches = count_equal(n2, end2, v);
                if (!matches)
                    return false;
                if (matches !=
                    1 + count_equal(node_algo::next_node(n1), end1, v))
                    return false;
            }
        }

        return true;
    }

    static bool find(node_pointer n, node_pointer end, value_type const& v)
    {
        for (; n != end; n = node_algo::next_node(n))
            if (n->value() == v)
                return true;
        return false;
    }

    static std::size_t count_equal(
        node_pointer n, node_pointer end, value_type const& v)
    {
        std::size_t count = 0;
        for (; n != end; n = node_algo::next_node(n))
            if (n->value() == v)
                ++count;
        return count;
    }

    // Emplace/Insert

    inline node_pointer add_node(
        node_pointer n, std::size_t key_hash, node_pointer pos)
    {
        n->hash_ = key_hash;
        if (pos) {
            node_algo::add_to_node_group(n, pos);
            if (n->next_) {
                std::size_t next_bucket =
                    this->hash_to_bucket(node_algo::next_node(n)->hash_);
                if (next_bucket != this->hash_to_bucket(key_hash)) {
                    this->get_bucket(next_bucket)->next_ = n;
                }
            }
        } else {
            bucket_pointer b = this->get_bucket(this->hash_to_bucket(key_hash));

            if (!b->next_) {
                link_pointer start_node = this->get_previous_start();

                if (start_node->next_) {
                    this->get_bucket(
                            this->hash_to_bucket(
                                node_algo::next_node(start_node)->hash_))
                        ->next_ = n;
                }

                b->next_ = start_node;
                n->next_ = start_node->next_;
                start_node->next_ = n;
            } else {
                n->next_ = b->next_->next_;
                b->next_->next_ = n;
            }
        }
        ++this->size_;
        return n;
    }

    inline node_pointer add_using_hint(node_pointer n, node_pointer hint)
    {
        n->hash_ = hint->hash_;
        node_algo::add_to_node_group(n, hint);
        if (n->next_ != hint && n->next_) {
            std::size_t next_bucket =
                this->hash_to_bucket(node_algo::next_node(n)->hash_);
            if (next_bucket != this->hash_to_bucket(n->hash_)) {
                this->get_bucket(next_bucket)->next_ = n;
            }
        }
        ++this->size_;
        return n;
    }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    iterator emplace(boost::unordered::detail::emplace_args1<
        boost::unordered::detail::please_ignore_this_overload> const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }

    iterator emplace_hint(
        c_iterator,
        boost::unordered::detail::emplace_args1<
            boost::unordered::detail::please_ignore_this_overload> const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }
#else
    iterator emplace(
        boost::unordered::detail::please_ignore_this_overload const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }

    iterator emplace_hint(c_iterator,
        boost::unordered::detail::please_ignore_this_overload const&)
    {
        BOOST_ASSERT(false);
        return iterator();
    }
#endif
#endif

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator emplace(BOOST_UNORDERED_EMPLACE_ARGS)
    {
        return iterator(emplace_impl(
            boost::unordered::detail::func::construct_node_from_args(
                this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD)));
    }

    template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
    iterator emplace_hint(c_iterator hint, BOOST_UNORDERED_EMPLACE_ARGS)
    {
        return iterator(emplace_hint_impl(
            hint, boost::unordered::detail::func::construct_node_from_args(
                      this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD)));
    }

    iterator emplace_impl(node_pointer n)
    {
        node_tmp a(n, this->node_alloc());
        const_key_type& k = this->get_key(a.node_->value());
        std::size_t key_hash = this->hash(k);
        node_pointer position = this->find_node(key_hash, k);
        this->reserve_for_insert(this->size_ + 1);
        return iterator(this->add_node(a.release(), key_hash, position));
    }

    iterator emplace_hint_impl(c_iterator hint, node_pointer n)
    {
        node_tmp a(n, this->node_alloc());
        const_key_type& k = this->get_key(a.node_->value());
        if (hint.node_ && this->key_eq()(k, this->get_key(*hint))) {
            this->reserve_for_insert(this->size_ + 1);
            return iterator(this->add_using_hint(a.release(), hint.node_));
        } else {
            std::size_t key_hash = this->hash(k);
            node_pointer position = this->find_node(key_hash, k);
            this->reserve_for_insert(this->size_ + 1);
            return iterator(this->add_node(a.release(), key_hash, position));
        }
    }

    void emplace_impl_no_rehash(node_pointer n)
    {
        node_tmp a(n, this->node_alloc());
        const_key_type& k = this->get_key(a.node_->value());
        std::size_t key_hash = this->hash(k);
        node_pointer position = this->find_node(key_hash, k);
        this->add_node(a.release(), key_hash, position);
    }

    template <typename NodeType> iterator move_insert_node_type(NodeType& np)
    {
        iterator result;

        if (np) {
            const_key_type& k = this->get_key(np.ptr_->value());
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);
            this->reserve_for_insert(this->size_ + 1);
            result = iterator(this->add_node(np.ptr_, key_hash, pos));
            np.ptr_ = node_pointer();
        }

        return result;
    }

    template <typename NodeType>
    iterator move_insert_node_type_with_hint(c_iterator hint, NodeType& np)
    {
        iterator result;

        if (np) {
            const_key_type& k = this->get_key(np.ptr_->value());

            if (hint.node_ && this->key_eq()(k, this->get_key(*hint))) {
                this->reserve_for_insert(this->size_ + 1);
                result = iterator(this->add_using_hint(np.ptr_, hint.node_));
            } else {
                std::size_t key_hash = this->hash(k);
                node_pointer pos = this->find_node(key_hash, k);
                this->reserve_for_insert(this->size_ + 1);
                result = iterator(this->add_node(np.ptr_, key_hash, pos));
            }
            np.ptr_ = node_pointer();
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////
    // Insert range methods

    // if hash function throws, or inserting > 1 element, basic exception
    // safety. Strong otherwise
    template <class I>
    void insert_range(I i, I j,
        typename boost::unordered::detail::enable_if_forward<I, void*>::type =
            0)
    {
        if (i == j)
            return;

        std::size_t distance = static_cast<std::size_t>(std::distance(i, j));
        if (distance == 1) {
            emplace_impl(boost::unordered::detail::func::construct_node(
                this->node_alloc(), *i));
        } else {
            // Only require basic exception safety here
            this->reserve_for_insert(this->size_ + distance);

            for (; i != j; ++i) {
                emplace_impl_no_rehash(
                    boost::unordered::detail::func::construct_node(
                        this->node_alloc(), *i));
            }
        }
    }

    template <class I>
    void insert_range(I i, I j,
        typename boost::unordered::detail::disable_if_forward<I, void*>::type =
            0)
    {
        for (; i != j; ++i) {
            emplace_impl(boost::unordered::detail::func::construct_node(
                this->node_alloc(), *i));
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Extract

    inline node_pointer extract_by_iterator(c_iterator n)
    {
        node_pointer i = n.node_;
        BOOST_ASSERT(i);
        node_pointer j(node_algo::next_node(i));
        std::size_t bucket_index = this->hash_to_bucket(i->hash_);
        // Split the groups containing 'i' and 'j'.
        // And get the pointer to the node before i while
        // we're at it.
        link_pointer prev = node_algo::split_groups(i, j);

        // If we don't have a 'prev' it means that i is at the
        // beginning of a block, so search through the blocks in the
        // same bucket.
        if (!prev) {
            prev = this->get_previous_start(bucket_index);
            while (prev->next_ != i) {
                prev = node_algo::next_for_erase(prev);
            }
        }

        prev->next_ = i->next_;
        --this->size_;
        this->fix_bucket(bucket_index, prev);
        i->next_ = link_pointer();

        return i;
    }

    ////////////////////////////////////////////////////////////////////////
    // Erase
    //
    // no throw

    std::size_t erase_key(const_key_type& k)
    {
        if (!this->size_)
            return 0;

        std::size_t key_hash = this->hash(k);
        std::size_t bucket_index = this->hash_to_bucket(key_hash);
        link_pointer prev = this->find_previous_node(k, key_hash, bucket_index);
        if (!prev)
            return 0;

        node_pointer first_node = node_algo::next_node(prev);
        link_pointer end = node_algo::next_group(first_node, this);

        std::size_t deleted_count = this->delete_nodes(prev, end);
        this->fix_bucket(bucket_index, prev);
        return deleted_count;
    }

    iterator erase(c_iterator r)
    {
        BOOST_ASSERT(r.node_);
        node_pointer next = node_algo::next_node(r.node_);
        erase_nodes(r.node_, next);
        return iterator(next);
    }

    iterator erase_range(c_iterator r1, c_iterator r2)
    {
        if (r1 == r2)
            return iterator(r2.node_);
        erase_nodes(r1.node_, r2.node_);
        return iterator(r2.node_);
    }

    link_pointer erase_nodes(node_pointer i, node_pointer j)
    {
        std::size_t bucket_index = this->hash_to_bucket(i->hash_);

        // Split the groups containing 'i' and 'j'.
        // And get the pointer to the node before i while
        // we're at it.
        link_pointer prev = node_algo::split_groups(i, j);

        // If we don't have a 'prev' it means that i is at the
        // beginning of a block, so search through the blocks in the
        // same bucket.
        if (!prev) {
            prev = this->get_previous_start(bucket_index);
            while (prev->next_ != i) {
                prev = node_algo::next_for_erase(prev);
            }
        }

        // Delete the nodes.
        // Is it inefficient to call fix_bucket for every node?
        do {
            this->delete_node(prev);
            bucket_index = this->fix_bucket(bucket_index, prev);
        } while (prev->next_ != j);

        return prev;
    }

    ////////////////////////////////////////////////////////////////////////
    // fill_buckets

    void copy_buckets(table const& src)
    {
        this->create_buckets(this->bucket_count_);

        for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = n->hash_;
            node_pointer group_end(node_algo::next_group(n, this));
            node_pointer pos =
                this->add_node(boost::unordered::detail::func::construct_node(
                                   this->node_alloc(), n->value()),
                    key_hash, node_pointer());
            for (n = node_algo::next_node(n); n != group_end;
                 n = node_algo::next_node(n)) {
                this->add_node(boost::unordered::detail::func::construct_node(
                                   this->node_alloc(), n->value()),
                    key_hash, pos);
            }
        }
    }

    void move_buckets(table const& src)
    {
        this->create_buckets(this->bucket_count_);

        for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = n->hash_;
            node_pointer group_end(node_algo::next_group(n, this));
            node_pointer pos =
                this->add_node(boost::unordered::detail::func::construct_node(
                                   this->node_alloc(), boost::move(n->value())),
                    key_hash, node_pointer());
            for (n = node_algo::next_node(n); n != group_end;
                 n = node_algo::next_node(n)) {
                this->add_node(boost::unordered::detail::func::construct_node(
                                   this->node_alloc(), boost::move(n->value())),
                    key_hash, pos);
            }
        }
    }

    void assign_buckets(table const& src)
    {
        node_holder<node_allocator> holder(*this);
        for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = n->hash_;
            node_pointer group_end(node_algo::next_group(n, this));
            node_pointer pos = this->add_node(
                holder.copy_of(n->value()), key_hash, node_pointer());
            for (n = node_algo::next_node(n); n != group_end;
                 n = node_algo::next_node(n)) {
                this->add_node(holder.copy_of(n->value()), key_hash, pos);
            }
        }
    }

    void move_assign_buckets(table& src)
    {
        node_holder<node_allocator> holder(*this);
        for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = n->hash_;
            node_pointer group_end(node_algo::next_group(n, this));
            node_pointer pos = this->add_node(
                holder.move_copy_of(n->value()), key_hash, node_pointer());
            for (n = node_algo::next_node(n); n != group_end;
                 n = node_algo::next_node(n)) {
                this->add_node(holder.move_copy_of(n->value()), key_hash, pos);
            }
        }
    }
};
}
}
}

#endif
