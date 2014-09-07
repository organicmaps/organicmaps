// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014 Samuel Debionne, Grenoble, France.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP
#define BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP

#include <boost/mpl/fold.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bind.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/placeholders.hpp>

#include <boost/type_traits.hpp>


namespace boost { namespace geometry
{

namespace util
{


/*!
    \brief Meta-function to generate all the combination of pairs of types
        from a given sequence Sequence except those that does not satisfy the
        predicate Pred
    \ingroup utility
    \par Example
    \code
        typedef mpl::vector<mpl::int_<0>, mpl::int_<1> > types;
        typedef combine_if<types, types, always<true_> >::type combinations;
        typedef mpl::vector<
            pair<mpl::int_<1>, mpl::int_<1> >,
            pair<mpl::int_<1>, mpl::int_<0> >,
            pair<mpl::int_<0>, mpl::int_<1> >,
            pair<mpl::int_<0>, mpl::int_<0> >        
        > result_types;
        
        BOOST_MPL_ASSERT(( mpl::equal<combinations, result_types> ));
    \endcode
*/
template <typename Sequence1, typename Sequence2, typename Pred>
struct combine_if
{
    struct combine
    {
        template <typename Result, typename T>
        struct apply
        {
            typedef typename mpl::fold<Sequence2, Result,
                mpl::if_
                <
                    mpl::bind<typename mpl::lambda<Pred>::type, T, mpl::_2>,
                    mpl::insert<mpl::_1, mpl::pair<T, mpl::_2> >,
                    mpl::_1
                >
            >::type type;
        };
    };

    typedef typename mpl::fold<Sequence1, mpl::set0<>, combine>::type type;
};


} // namespace util

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP
