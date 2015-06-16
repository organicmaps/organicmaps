// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_POINT_ON_LINE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_POINT_ON_LINE_HPP


#include <boost/geometry/algorithms/distance.hpp>

namespace boost { namespace geometry
{

//----------------------------------------------------------------------
// Function     : point_on_linestring -> rename to alongLine NO, different
//----------------------------------------------------------------------
// Purpose      : Calculates coordinates of a point along a given line
//                on a specified distance
// Parameters   : const L& : line,
//                float position: position to calculate point
//                P& point: point to calculate
// Return       : true if point lies on line
//----------------------------------------------------------------------
// Author       : Barend, Geodan BV Amsterdam
// Date         : spring 1996
//----------------------------------------------------------------------
template <typename P, typename L>
bool point_on_linestring(L const& line, double const& position, P& point)
{
    double current_distance = 0.0;
    if (line.size() < 2)
    {
        return false;
    }

    typename L::const_iterator vertex = line.begin();
    typename L::const_iterator previous = vertex++;

    while (vertex != line.end())
    {
        double const dist = distance(*previous, *vertex);
        current_distance += dist;

        if (current_distance > position)
        {
            // It is not possible that dist == 0 here because otherwise
            // the current_distance > position would not become true (current_distance is increased by dist)
            double const fraction = 1.0 - ((current_distance - position) / dist);

            // point i is too far, point i-1 to near, add fraction of
            // distance in each direction
            point.x ( previous->x() + (vertex->x() - previous->x()) * fraction);
            point.y ( previous->y() + (vertex->y() - previous->y()) * fraction);

            return true;
        }
        previous = vertex++;
    }

    // point at specified position does not lie on line
    return false;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_POINT_ON_LINE_HPP
