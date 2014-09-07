// (C) Copyright David Abrahams 2002.
// (C) Copyright Jeremy Siek    2002.
// (C) Copyright Thomas Witt    2002.
// (C) Copyright Jeffrey Lee Hellrung, Jr. 2012.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_OPERATOR_BRACKETS_DISPATCH_07102012JLH_HPP
#define BOOST_OPERATOR_BRACKETS_DISPATCH_07102012JLH_HPP

#include <boost/iterator/detail/facade_iterator_category.hpp>

#include <boost/type_traits/is_pod.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/mpl/if.hpp>

namespace boost { namespace detail {

// operator[] must return a proxy in case iterator destruction invalidates
// referents.
// To see why, consider the following implementation of operator[]:
//   reference operator[](difference_type n) const
//   { return *(*this + n); }
// The problem here is that operator[] would return a reference created from
// a temporary iterator.

template <class Value>
struct operator_brackets_value
{
    typedef Value result_type;
    template <class Iterator>
    static result_type apply(Iterator const & i)
    { return *i; }
};

template <class Iterator, class Reference>
struct operator_brackets_const_proxy
{
    class result_type
    {
        Iterator const m_i;
        explicit result_type(Iterator const & i) : m_i(i) { }
        friend struct operator_brackets_const_proxy;
        void operator=(result_type&);
    public:
        operator Reference() const { return *m_i; }
    };
    static result_type apply(Iterator const & i)
    { return result_type(i); }
};

template <class Iterator, class Reference>
struct operator_brackets_proxy
{
    class result_type
    {
        Iterator const m_i;
        explicit result_type(Iterator const & i) : m_i(i) { }
        friend struct operator_brackets_proxy;
        void operator=(result_type&);
    public:
        operator Reference() const { return *m_i; }
        operator_brackets_proxy const & operator=(
          typename Iterator::value_type const & x) const
        { *m_i = x; return *this; }
    };
    static result_type apply(Iterator const & i)
    { return result_type(i); }
};

template <class Iterator, class ValueType, class Reference>
struct operator_brackets_dispatch
{
    typedef typename mpl::if_c<
        iterator_writability_disabled<ValueType,Reference>::value,
        typename mpl::if_c<
            boost::is_POD<ValueType>::value,
            operator_brackets_value<typename boost::remove_const<ValueType>::type>,
            operator_brackets_const_proxy<Iterator,Reference>
        >::type,
        operator_brackets_proxy<Iterator,Reference>
    >::type type;
};

} } // namespace detail / namespace boost

#endif // #ifndef BOOST_OPERATOR_BRACKETS_DISPATCH_07102012JLH_HPP
