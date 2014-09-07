// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_COMPRESS_VARIANT_HPP
#define BOOST_GEOMETRY_UTIL_COMPRESS_VARIANT_HPP


#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant/variant_fwd.hpp>


namespace boost { namespace geometry
{


namespace detail
{

template <typename Variant>
struct unique_types:
    mpl::fold<
        typename mpl::reverse_fold<
            typename Variant::types,
            mpl::set<>,
            mpl::insert<
                mpl::placeholders::_1,
                mpl::placeholders::_2
            >
        >::type,
        mpl::vector<>,
        mpl::push_back<mpl::placeholders::_1, mpl::placeholders::_2>
    >
{};

template <typename Types>
struct variant_or_single:
    mpl::if_<
        mpl::equal_to<
            mpl::size<Types>,
            mpl::int_<1>
        >,
        typename mpl::front<Types>::type,
        typename make_variant_over<Types>::type
    >
{};

} // namespace detail


/*!
    \brief Meta-function that takes a boost::variant type and tries to minimize
        it by doing the following:
        - if there's any duplicate types, remove them
        - if the result is a variant of one type, turn it into just that type
    \ingroup utility
    \par Example
    \code
        typedef variant<int, float, int, long> variant_type;
        typedef compress_variant<variant_type>::type compressed;
        typedef mpl::vector<int, float, long> result_types;
        BOOST_MPL_ASSERT(( mpl::equal<compressed::types, result_types> ));

        typedef variant<int, int, int> one_type_variant_type;
        typedef compress_variant<one_type_variant_type>::type single_type;
        BOOST_MPL_ASSERT(( boost::equals<single_type, int> ));
    \endcode
*/

template <typename Variant>
struct compress_variant:
    detail::variant_or_single<
        typename detail::unique_types<Variant>::type
    >
{};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_COMPRESS_VARIANT_HPP
