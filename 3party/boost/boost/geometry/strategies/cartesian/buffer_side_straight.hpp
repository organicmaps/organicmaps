// Boost.Geometry (aka GGL, Generic Geometry Library)
// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP

#include <cstddef>

#include <boost/assert.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/strategies/side.hpp>



namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{



/*!
\brief Let the buffer use straight sides along segments (the default)
\ingroup strategies
\details This strategy can be used as SideStrategy for the buffer algorithm.
    It is currently the only provided strategy for this purpose

\qbk{
[heading Example]
See the examples for other buffer strategies\, for example
[link geometry.reference.strategies.strategy_buffer_join_round join_round]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
}
 */
class side_straight
{
public :
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template
    <
        typename Point,
        typename OutputRange,
        typename DistanceStrategy
    >
    static inline void apply(
                Point const& input_p1, Point const& input_p2,
                strategy::buffer::buffer_side_selector side,
                DistanceStrategy const& distance,
                OutputRange& output_range)
    {
        typedef typename coordinate_type<Point>::type coordinate_type;
        typedef typename geometry::select_most_precise
        <
            coordinate_type,
            double
        >::type promoted_type;

        // Generate a block along (left or right of) the segment

        // Simulate a vector d (dx,dy)
        coordinate_type dx = get<0>(input_p2) - get<0>(input_p1);
        coordinate_type dy = get<1>(input_p2) - get<1>(input_p1);

        // For normalization [0,1] (=dot product d.d, sqrt)
        promoted_type const length = geometry::math::sqrt(dx * dx + dy * dy);

        // Because coordinates are not equal, length should not be zero
        BOOST_ASSERT((! geometry::math::equals(length, 0)));

        // Generate the normalized perpendicular p, to the left (ccw)
        promoted_type const px = -dy / length;
        promoted_type const py = dx / length;

        promoted_type const d = distance.apply(input_p1, input_p2, side);

        output_range.resize(2);

        set<0>(output_range.front(), get<0>(input_p1) + px * d);
        set<1>(output_range.front(), get<1>(input_p1) + py * d);
        set<0>(output_range.back(), get<0>(input_p2) + px * d);
        set<1>(output_range.back(), get<1>(input_p2) + py * d);
    }
#endif // DOXYGEN_SHOULD_SKIP_THIS
};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP
