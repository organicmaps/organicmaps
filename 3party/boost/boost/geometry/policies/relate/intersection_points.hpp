// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP
#define BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP


#include <algorithm>
#include <string>

#include <boost/concept_check.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/util/promote_integral.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace policies { namespace relate
{


/*!
\brief Policy calculating the intersection points themselves
 */
template
<
    typename ReturnType
>
struct segments_intersection_points
{
    typedef ReturnType return_type;

    template
    <
        typename Point,
        typename Segment,
        typename SegmentRatio,
        typename T
    >
    static inline void assign(Point& point,
                Segment const& segment,
                SegmentRatio const& ratio,
                T const& dx, T const& dy)
    {
        typedef typename geometry::coordinate_type<Point>::type coordinate_type;

        // Calculate the intersection point based on segment_ratio
        // Up to now, division was postponed. Here we divide using numerator/
        // denominator. In case of integer this results in an integer
        // division.
        BOOST_GEOMETRY_ASSERT(ratio.denominator() != 0);

        typedef typename promote_integral<coordinate_type>::type promoted_type;

        promoted_type const numerator
            = boost::numeric_cast<promoted_type>(ratio.numerator());
        promoted_type const denominator
            = boost::numeric_cast<promoted_type>(ratio.denominator());
        promoted_type const dx_promoted = boost::numeric_cast<promoted_type>(dx);
        promoted_type const dy_promoted = boost::numeric_cast<promoted_type>(dy);

        set<0>(point, get<0, 0>(segment) + boost::numeric_cast
            <
                coordinate_type
            >(numerator * dx_promoted / denominator));
        set<1>(point, get<0, 1>(segment) + boost::numeric_cast
            <
                coordinate_type
            >(numerator * dy_promoted / denominator));
    }

    template
    <
        typename Segment1,
        typename Segment2,
        typename SegmentIntersectionInfo
    >
    static inline return_type segments_crosses(side_info const&,
                    SegmentIntersectionInfo const& sinfo,
                    Segment1 const& s1, Segment2 const& s2)
    {
        return_type result;
        result.count = 1;

        bool use_a = true;

        // Prefer one segment if one is on or near an endpoint
        bool const a_near_end = sinfo.robust_ra.near_end();
        bool const b_near_end = sinfo.robust_rb.near_end();
        if (a_near_end && ! b_near_end)
        {
            use_a = true;
        }
        else if (b_near_end && ! a_near_end)
        {
            use_a = false;
        }
        else
        {
            // Prefer shorter segment
            typedef typename SegmentIntersectionInfo::promoted_type ptype;
            ptype const len_a = sinfo.dx_a * sinfo.dx_a + sinfo.dy_a * sinfo.dy_a;
            ptype const len_b = sinfo.dx_b * sinfo.dx_b + sinfo.dy_b * sinfo.dy_b;
            if (len_b < len_a)
            {
                use_a = false;
            }
            // else use_a is true but was already assigned like that
        }

        if (use_a)
        {
            assign(result.intersections[0], s1, sinfo.robust_ra,
                sinfo.dx_a, sinfo.dy_a);
        }
        else
        {
            assign(result.intersections[0], s2, sinfo.robust_rb,
                sinfo.dx_b, sinfo.dy_b);
        }

        result.fractions[0].assign(sinfo);

        return result;
    }

    template <typename Segment1, typename Segment2, typename Ratio>
    static inline return_type segments_collinear(
        Segment1 const& a, Segment2 const& b, bool /*opposite*/,
        int a1_wrt_b, int a2_wrt_b, int b1_wrt_a, int b2_wrt_a,
        Ratio const& ra_from_wrt_b, Ratio const& ra_to_wrt_b,
        Ratio const& rb_from_wrt_a, Ratio const& rb_to_wrt_a)
    {
        return_type result;
        unsigned int index = 0, count_a = 0, count_b = 0;
        Ratio on_a[2];

        // The conditions "index < 2" are necessary for non-robust handling,
        // if index would be 2 this indicate an (currently uncatched) error

        // IMPORTANT: the order of conditions is different as in direction.hpp
        if (a1_wrt_b >= 1 && a1_wrt_b <= 3 // ra_from_wrt_b.on_segment()
            && index < 2)
        {
            //     a1--------->a2
            // b1----->b2
            //
            // ra1 (relative to b) is between 0/1:
            // -> First point of A is intersection point
            detail::assign_point_from_index<0>(a, result.intersections[index]);
            result.fractions[index].assign(Ratio::zero(), ra_from_wrt_b);
            on_a[index] = Ratio::zero();
            index++;
            count_a++;
        }
        if (b1_wrt_a == 2 //rb_from_wrt_a.in_segment()
            && index < 2)
        {
            // We take the first intersection point of B
            // a1--------->a2
            //         b1----->b2
            // But only if it is not located on A
            // a1--------->a2
            // b1----->b2      rb_from_wrt_a == 0/1 -> a already taken

            detail::assign_point_from_index<0>(b, result.intersections[index]);
            result.fractions[index].assign(rb_from_wrt_a, Ratio::zero());
            on_a[index] = rb_from_wrt_a;
            index++;
            count_b++;
        }

        if (a2_wrt_b >= 1 && a2_wrt_b <= 3 //ra_to_wrt_b.on_segment()
            && index < 2)
        {
            // Similarly, second IP (here a2)
            // a1--------->a2
            //         b1----->b2
            detail::assign_point_from_index<1>(a, result.intersections[index]);
            result.fractions[index].assign(Ratio::one(), ra_to_wrt_b);
            on_a[index] = Ratio::one();
            index++;
            count_a++;
        }
        if (b2_wrt_a == 2 // rb_to_wrt_a.in_segment()
            && index < 2)
        {
            detail::assign_point_from_index<1>(b, result.intersections[index]);
            result.fractions[index].assign(rb_to_wrt_a, Ratio::one());
            on_a[index] = rb_to_wrt_a;
            index++;
            count_b++;
        }

        // TEMPORARY
        // If both are from b, and b is reversed w.r.t. a, we swap IP's
        // to align them w.r.t. a
        // get_turn_info still relies on some order (in some collinear cases)
        if (index == 2 && on_a[1] < on_a[0])
        {
            std::swap(result.fractions[0], result.fractions[1]);
            std::swap(result.intersections[0], result.intersections[1]);
        }

        result.count = index;

        return result;
    }

    static inline return_type disjoint()
    {
        return return_type();
    }
    static inline return_type error(std::string const&)
    {
        return return_type();
    }

    // Both degenerate
    template <typename Segment>
    static inline return_type degenerate(Segment const& segment, bool)
    {
        return_type result;
        result.count = 1;
        set<0>(result.intersections[0], get<0, 0>(segment));
        set<1>(result.intersections[0], get<0, 1>(segment));
        return result;
    }

    // One degenerate
    template <typename Segment, typename Ratio>
    static inline return_type one_degenerate(Segment const& degenerate_segment,
            Ratio const& ratio, bool a_degenerate)
    {
        return_type result;
        result.count = 1;
        set<0>(result.intersections[0], get<0, 0>(degenerate_segment));
        set<1>(result.intersections[0], get<0, 1>(degenerate_segment));
        if (a_degenerate)
        {
            // IP lies on ratio w.r.t. segment b
            result.fractions[0].assign(Ratio::zero(), ratio);
        }
        else
        {
            result.fractions[0].assign(ratio, Ratio::zero());
        }
        return result;
    }
};


}} // namespace policies::relate

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_GEOMETRY_POLICIES_RELATE_INTERSECTION_POINTS_HPP
