// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_COVERED_BY_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_COVERED_BY_HPP


#include <boost/geometry/algorithms/covered_by.hpp>

#include <boost/geometry/extensions/nsphere/strategies/cartesian/nsphere_in_box.hpp>
#include <boost/geometry/extensions/nsphere/strategies/cartesian/point_in_nsphere.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename NSphere, typename Box>
struct covered_by<NSphere, Box, nsphere_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(NSphere const& nsphere, Box const& box, Strategy const& strategy)
    {
        assert_dimension_equal<NSphere, Box>();
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(nsphere, box);
    }
};

template <typename Point, typename NSphere>
struct covered_by<Point, NSphere, point_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(Point const& point, NSphere const& nsphere, Strategy const& strategy)
    {
        assert_dimension_equal<Point, NSphere>();
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(point, nsphere);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_COVERED_BY_HPP
