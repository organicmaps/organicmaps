// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP

#include <algorithm>

#include <boost/geometry/core/exception.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>

#include <boost/geometry/arithmetic/determinant.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

// Temporary / will be Strategy as template parameter
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>

#include <boost/geometry/strategies/side_info.hpp>
#include <boost/geometry/strategies/intersection_result.hpp>

#include <boost/geometry/policies/robustness/robust_point_type.hpp>
#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>


#if defined(BOOST_GEOMETRY_DEBUG_ROBUSTNESS)
#  include <boost/geometry/io/wkt/write.hpp>
#endif


namespace boost { namespace geometry
{


namespace strategy { namespace intersection
{


/*!
    \see http://mathworld.wolfram.com/Line-LineIntersection.html
 */
template <typename Policy, typename CalculationType = void>
struct relate_cartesian_segments
{
    typedef typename Policy::return_type return_type;

    template <typename D, typename W, typename ResultType>
    static inline void cramers_rule(D const& dx_a, D const& dy_a,
        D const& dx_b, D const& dy_b, W const& wx, W const& wy,
        // out:
        ResultType& d, ResultType& da)
    {
        // Cramers rule
        d = geometry::detail::determinant<ResultType>(dx_a, dy_a, dx_b, dy_b);
        da = geometry::detail::determinant<ResultType>(dx_b, dy_b, wx, wy);
        // Ratio is da/d , collinear if d == 0, intersecting if 0 <= r <= 1
        // IntersectionPoint = (x1 + r * dx_a, y1 + r * dy_a)
    }


    // Relate segments a and b
    template <typename Segment1, typename Segment2, typename RobustPolicy>
    static inline return_type apply(Segment1 const& a, Segment2 const& b,
                RobustPolicy const& robust_policy)
    {
        // type them all as in Segment1 - TODO reconsider this, most precise?
        typedef typename geometry::point_type<Segment1>::type point_type;

        typedef typename geometry::robust_point_type
            <
                point_type, RobustPolicy
            >::type robust_point_type;

        point_type a0, a1, b0, b1;
        robust_point_type a0_rob, a1_rob, b0_rob, b1_rob;

        detail::assign_point_from_index<0>(a, a0);
        detail::assign_point_from_index<1>(a, a1);
        detail::assign_point_from_index<0>(b, b0);
        detail::assign_point_from_index<1>(b, b1);

        geometry::recalculate(a0_rob, a0, robust_policy);
        geometry::recalculate(a1_rob, a1, robust_policy);
        geometry::recalculate(b0_rob, b0, robust_policy);
        geometry::recalculate(b1_rob, b1, robust_policy);

        return apply(a, b, robust_policy, a0_rob, a1_rob, b0_rob, b1_rob);
    }

    // The main entry-routine, calculating intersections of segments a / b
    template <typename Segment1, typename Segment2, typename RobustPolicy, typename RobustPoint>
    static inline return_type apply(Segment1 const& a, Segment2 const& b,
            RobustPolicy const& robust_policy,
            RobustPoint const& robust_a1, RobustPoint const& robust_a2,
            RobustPoint const& robust_b1, RobustPoint const& robust_b2)
    {
        BOOST_CONCEPT_ASSERT( (concept::ConstSegment<Segment1>) );
        BOOST_CONCEPT_ASSERT( (concept::ConstSegment<Segment2>) );

        boost::ignore_unused_variable_warning(robust_policy);

        typedef typename select_calculation_type
            <Segment1, Segment2, CalculationType>::type coordinate_type;

        using geometry::detail::equals::equals_point_point;
        bool const a_is_point = equals_point_point(robust_a1, robust_a2);
        bool const b_is_point = equals_point_point(robust_b1, robust_b2);

        typedef side::side_by_triangle<coordinate_type> side;

        if(a_is_point && b_is_point)
        {
            return equals_point_point(robust_a1, robust_b2)
                ? Policy::degenerate(a, true)
                : Policy::disjoint()
                ;
        }

        side_info sides;
        sides.set<0>(side::apply(robust_b1, robust_b2, robust_a1),
                     side::apply(robust_b1, robust_b2, robust_a2));
        sides.set<1>(side::apply(robust_a1, robust_a2, robust_b1),
                     side::apply(robust_a1, robust_a2, robust_b2));

        bool collinear = sides.collinear();

        if (sides.same<0>() || sides.same<1>())
        {
            // Both points are at same side of other segment, we can leave
            return Policy::disjoint();
        }

        typedef typename select_most_precise
            <
                coordinate_type, double
            >::type promoted_type;

        typedef typename geometry::coordinate_type
            <
                RobustPoint
            >::type robust_coordinate_type;

        typedef typename segment_ratio_type
        <
            typename geometry::point_type<Segment1>::type, // TODO: most precise point?
            RobustPolicy
        >::type ratio_type;

        segment_intersection_info
        <
            coordinate_type,
            promoted_type,
            ratio_type
        > sinfo;

        sinfo.dx_a = get<1, 0>(a) - get<0, 0>(a); // distance in x-dir
        sinfo.dx_b = get<1, 0>(b) - get<0, 0>(b);
        sinfo.dy_a = get<1, 1>(a) - get<0, 1>(a); // distance in y-dir
        sinfo.dy_b = get<1, 1>(b) - get<0, 1>(b);

        robust_coordinate_type const robust_dx_a = get<0>(robust_a2) - get<0>(robust_a1);
        robust_coordinate_type const robust_dx_b = get<0>(robust_b2) - get<0>(robust_b1);
        robust_coordinate_type const robust_dy_a = get<1>(robust_a2) - get<1>(robust_a1);
        robust_coordinate_type const robust_dy_b = get<1>(robust_b2) - get<1>(robust_b1);

        // r: ratio 0-1 where intersection divides A/B
        // (only calculated for non-collinear segments)
        if (! collinear)
        {
            robust_coordinate_type robust_da0, robust_da;
            robust_coordinate_type robust_db0, robust_db;

            cramers_rule(robust_dx_a, robust_dy_a, robust_dx_b, robust_dy_b,
                get<0>(robust_a1) - get<0>(robust_b1),
                get<1>(robust_a1) - get<1>(robust_b1),
                robust_da0, robust_da);

            cramers_rule(robust_dx_b, robust_dy_b, robust_dx_a, robust_dy_a,
                get<0>(robust_b1) - get<0>(robust_a1),
                get<1>(robust_b1) - get<1>(robust_a1),
                robust_db0, robust_db);

            if (robust_da0 == 0)
            {
                // If this is the case, no rescaling is done for FP precision.
                // We set it to collinear, but it indicates a robustness issue.
                sides.set<0>(0,0);
                sides.set<1>(0,0);
                collinear = true;
            }
            else
            {
                sinfo.robust_ra.assign(robust_da, robust_da0);
                sinfo.robust_rb.assign(robust_db, robust_db0);
            }
        }

        if (collinear)
        {
            bool const collinear_use_first
                    = geometry::math::abs(robust_dx_a) + geometry::math::abs(robust_dx_b)
                    >= geometry::math::abs(robust_dy_a) + geometry::math::abs(robust_dy_b);

            // Degenerate cases: segments of single point, lying on other segment, are not disjoint
            // This situation is collinear too

            if (collinear_use_first)
            {
                return relate_collinear<0, ratio_type>(a, b,
                        robust_a1, robust_a2, robust_b1, robust_b2,
                        a_is_point, b_is_point);
            }
            else
            {
                // Y direction contains larger segments (maybe dx is zero)
                return relate_collinear<1, ratio_type>(a, b,
                        robust_a1, robust_a2, robust_b1, robust_b2,
                        a_is_point, b_is_point);
            }
        }

        return Policy::segments_crosses(sides, sinfo, a, b);
    }

private:
    template
    <
        std::size_t Dimension,
        typename RatioType,
        typename Segment1,
        typename Segment2,
        typename RobustPoint
    >
    static inline return_type relate_collinear(Segment1 const& a,
            Segment2 const& b,
            RobustPoint const& robust_a1, RobustPoint const& robust_a2,
            RobustPoint const& robust_b1, RobustPoint const& robust_b2,
            bool a_is_point, bool b_is_point)
    {
        if (a_is_point)
        {
            return relate_one_degenerate<RatioType>(a,
                get<Dimension>(robust_a1),
                get<Dimension>(robust_b1), get<Dimension>(robust_b2),
                true);
        }
        if (b_is_point)
        {
            return relate_one_degenerate<RatioType>(b,
                get<Dimension>(robust_b1),
                get<Dimension>(robust_a1), get<Dimension>(robust_a2),
                false);
        }
        return relate_collinear<RatioType>(a, b,
                                get<Dimension>(robust_a1),
                                get<Dimension>(robust_a2),
                                get<Dimension>(robust_b1),
                                get<Dimension>(robust_b2));
    }

    /// Relate segments known collinear
    template
    <
        typename RatioType,
        typename Segment1,
        typename Segment2,
        typename RobustType
    >
    static inline return_type relate_collinear(Segment1 const& a
            , Segment2 const& b
            , RobustType oa_1, RobustType oa_2
            , RobustType ob_1, RobustType ob_2
            )
    {
        // Calculate the ratios where a starts in b, b starts in a
        //         a1--------->a2         (2..7)
        //                b1----->b2      (5..8)
        // length_a: 7-2=5
        // length_b: 8-5=3
        // b1 is located w.r.t. a at ratio: (5-2)/5=3/5 (on a)
        // b2 is located w.r.t. a at ratio: (8-2)/5=6/5 (right of a)
        // a1 is located w.r.t. b at ratio: (2-5)/3=-3/3 (left of b)
        // a2 is located w.r.t. b at ratio: (7-5)/3=2/3 (on b)
        // A arrives (a2 on b), B departs (b1 on a)

        // If both are reversed:
        //         a2<---------a1         (7..2)
        //                b2<-----b1      (8..5)
        // length_a: 2-7=-5
        // length_b: 5-8=-3
        // b1 is located w.r.t. a at ratio: (8-7)/-5=-1/5 (before a starts)
        // b2 is located w.r.t. a at ratio: (5-7)/-5=2/5 (on a)
        // a1 is located w.r.t. b at ratio: (7-8)/-3=1/3 (on b)
        // a2 is located w.r.t. b at ratio: (2-8)/-3=6/3 (after b ends)

        // If both one is reversed:
        //         a1--------->a2         (2..7)
        //                b2<-----b1      (8..5)
        // length_a: 7-2=+5
        // length_b: 5-8=-3
        // b1 is located w.r.t. a at ratio: (8-2)/5=6/5 (after a ends)
        // b2 is located w.r.t. a at ratio: (5-2)/5=3/5 (on a)
        // a1 is located w.r.t. b at ratio: (2-8)/-3=6/3 (after b ends)
        // a2 is located w.r.t. b at ratio: (7-8)/-3=1/3 (on b)
        RobustType const length_a = oa_2 - oa_1; // no abs, see above
        RobustType const length_b = ob_2 - ob_1;

        RatioType const ra_from(oa_1 - ob_1, length_b);
        RatioType const ra_to(oa_2 - ob_1, length_b);
        RatioType const rb_from(ob_1 - oa_1, length_a);
        RatioType const rb_to(ob_2 - oa_1, length_a);

        if ((ra_from.left() && ra_to.left()) || (ra_from.right() && ra_to.right()))
        {
            return Policy::disjoint();
        }

        return Policy::segments_collinear(a, b, ra_from, ra_to, rb_from, rb_to);
    }

    /// Relate segments where one is degenerate
    template
    <
        typename RatioType,
        typename DegenerateSegment,
        typename RobustType
    >
    static inline return_type relate_one_degenerate(
            DegenerateSegment const& degenerate_segment
            , RobustType d
            , RobustType s1, RobustType s2
            , bool a_degenerate
            )
    {
        // Calculate the ratios where ds starts in s
        //         a1--------->a2         (2..6)
        //              b1/b2      (4..4)
        // Ratio: (4-2)/(6-2)
        RatioType const ratio(d - s1, s2 - s1);
        return Policy::one_degenerate(degenerate_segment, ratio, a_degenerate);
    }
};


}} // namespace strategy::intersection

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP
