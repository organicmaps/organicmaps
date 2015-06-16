// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ASSIGN_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ASSIGN_HPP

#include <boost/geometry/algorithms/assign.hpp>

#include <boost/geometry/extensions/nsphere/core/tags.hpp>
#include <boost/geometry/extensions/nsphere/core/radius.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename S>
struct assign<nsphere_tag, S, 2>
{
    typedef typename coordinate_type<S>::type coordinate_type;
    typedef typename radius_type<S>::type radius_type;

    /// 2-value version for an n-sphere is valid for circle and sets the center
    template <typename T>
    static inline void apply(S& sphercle, T const& c1, T const& c2)
    {
        set<0>(sphercle, boost::numeric_cast<coordinate_type>(c1));
        set<1>(sphercle, boost::numeric_cast<coordinate_type>(c2));
    }

    template <typename T, typename R>
    static inline void apply(S& sphercle, T const& c1,
        T const& c2, R const& radius)
    {
        set<0>(sphercle, boost::numeric_cast<coordinate_type>(c1));
        set<1>(sphercle, boost::numeric_cast<coordinate_type>(c2));
        set_radius<0>(sphercle, boost::numeric_cast<radius_type>(radius));
    }
};

template <typename S>
struct assign<nsphere_tag, S, 3>
{
    typedef typename coordinate_type<S>::type coordinate_type;
    typedef typename radius_type<S>::type radius_type;

    /// 4-value version for an n-sphere is valid for a sphere and sets the center and the radius
    template <typename T>
    static inline void apply(S& sphercle, T const& c1, T const& c2, T const& c3)
    {
        set<0>(sphercle, boost::numeric_cast<coordinate_type>(c1));
        set<1>(sphercle, boost::numeric_cast<coordinate_type>(c2));
        set<2>(sphercle, boost::numeric_cast<coordinate_type>(c3));
    }

    /// 4-value version for an n-sphere is valid for a sphere and sets the center and the radius
    template <typename T, typename R>
    static inline void apply(S& sphercle, T const& c1,
        T const& c2, T const& c3, R const& radius)
    {

        set<0>(sphercle, boost::numeric_cast<coordinate_type>(c1));
        set<1>(sphercle, boost::numeric_cast<coordinate_type>(c2));
        set<2>(sphercle, boost::numeric_cast<coordinate_type>(c3));
        set_radius<0>(sphercle, boost::numeric_cast<radius_type>(radius));
    }

};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ASSIGN_HPP
