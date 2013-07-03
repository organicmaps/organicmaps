// Boost.Geometry Index
//
// Type-erased iterators
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_TYPE_ERASED_ITERATORS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_TYPE_ERASED_ITERATORS_HPP

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/operators.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template<typename T, typename ValueType, typename Reference, typename Pointer, typename DifferenceType>
struct single_pass_iterator_concept :
    ::boost::mpl::vector<
        ::boost::type_erasure::copy_constructible<T>,
        ::boost::type_erasure::equality_comparable<T>,
        ::boost::type_erasure::dereferenceable<Reference, T>,
        ::boost::type_erasure::assignable<T>,
        ::boost::type_erasure::incrementable<T>
    >
{};

template <typename ValueType, typename Reference, typename Pointer, typename DifferenceType>
struct single_pass_iterator_type
{
    typedef ::boost::type_erasure::any<
        single_pass_iterator_concept<
            ::boost::type_erasure::_self, ValueType, Reference, Pointer, DifferenceType
        >
    > type;
};

}}}} // namespace boost::geometry::index::detail

namespace boost { namespace type_erasure {

template<typename T, typename ValueType, typename Reference, typename Pointer, typename DifferenceType, typename Base>
struct concept_interface<
    ::boost::geometry::index::detail::single_pass_iterator_concept<
        T, ValueType, Reference, Pointer, DifferenceType
    >, Base, T>
    : Base
{
    typedef ValueType value_type;
    typedef Reference reference;
    typedef Pointer pointer;
    typedef DifferenceType difference_type;
    typedef ::std::input_iterator_tag iterator_category;
};

}} // namespace boost::type_erasure

#endif // BOOST_GEOMETRY_INDEX_DETAIL_TYPE_ERASED_ITERATORS_HPP
