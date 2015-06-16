// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012 Krzysztof Czainski

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_PROJECTION_POINT_TYPE_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PROJECTION_POINT_TYPE_HPP

namespace boost { namespace geometry { namespace projections
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Projection, typename CoordinateSystemTag>
struct projection_point_type
{};

template <typename Projection>
struct projection_point_type<Projection, cartesian_tag>
{
    typedef typename Projection::cartesian_point_type type;
};

template <typename Projection>
struct projection_point_type<Projection, geographic_tag>
{
    typedef typename Projection::geographic_point_type type;
};

} // detail
#endif // DOXYGEN_NO_DETAIL

}}} // boost::geometry::projection

#endif // BOOST_GEOMETRY_PROJECTIONS_PROJECTION_POINT_TYPE_HPP
