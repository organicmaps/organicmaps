// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CALCULATE_DISTANCE_POLICY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CALCULATE_DISTANCE_POLICY_HPP


#include <boost/geometry/algorithms/distance.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


/*!
    \brief Policy calculating distance
    \details get_turn_info has an optional policy to get some
        extra information.
        This policy calculates the distance (using default distance strategy)
 */
struct calculate_distance_policy
{
    template <typename Point1, typename Point2, typename Info>
    static inline void apply(Info& info, Point1 const& p1, Point2 const& p2)
    {
        info.operations[0].enriched.distance
                    = geometry::distance(info.point, p1);
        info.operations[1].enriched.distance
                    = geometry::distance(info.point, p2);
    }

};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CALCULATE_DISTANCE_POLICY_HPP
