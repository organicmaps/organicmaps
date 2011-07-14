// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP

#include <algorithm>

#include <boost/geometry/core/exception.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>
#include <boost/geometry/geometries/concepts/segment_concept.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

// Temporary / will be Strategy as template parameter
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/cartesian/side_by_triangle.hpp>

#include <boost/geometry/strategies/side_info.hpp>


namespace boost { namespace geometry
{


namespace strategy { namespace intersection
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Segment, size_t Dimension>
struct segment_arrange
{
    template <typename T>
    static inline void apply(Segment const& s, T& s_1, T& s_2, bool& swapped)
    {
        s_1 = get<0, Dimension>(s);
        s_2 = get<1, Dimension>(s);
        if (s_1 > s_2)
        {
            std::swap(s_1, s_2);
            swapped = true;
        }
    }
};

}
#endif

/***
template <typename T>
inline std::string rdebug(T const& value)
{
    if (math::equals(value, 0)) return "'0'";
    if (math::equals(value, 1)) return "'1'";
    if (value < 0) return "<0";
    if (value > 1) return ">1";
    return "<0..1>";
}
***/

/*!
    \see http://mathworld.wolfram.com/Line-LineIntersection.html
 */
template <typename Policy, typename CalculationType = void>
struct relate_cartesian_segments
{
    typedef typename Policy::return_type return_type;
    typedef typename Policy::segment_type1 segment_type1;
    typedef typename Policy::segment_type2 segment_type2;

    //typedef typename point_type<segment_type1>::type point_type;
    //BOOST_CONCEPT_ASSERT( (concept::Point<point_type>) );

    BOOST_CONCEPT_ASSERT( (concept::ConstSegment<segment_type1>) );
    BOOST_CONCEPT_ASSERT( (concept::ConstSegment<segment_type2>) );

    typedef typename select_calculation_type
        <segment_type1, segment_type2, CalculationType>::type coordinate_type;

    /// Relate segments a and b
    static inline return_type apply(segment_type1 const& a, segment_type2 const& b)
    {
        coordinate_type dx_a = get<1, 0>(a) - get<0, 0>(a); // distance in x-dir
        coordinate_type dx_b = get<1, 0>(b) - get<0, 0>(b);
        coordinate_type dy_a = get<1, 1>(a) - get<0, 1>(a); // distance in y-dir
        coordinate_type dy_b = get<1, 1>(b) - get<0, 1>(b);
        return apply(a, b, dx_a, dy_a, dx_b, dy_b);
    }


    // Relate segments a and b using precalculated differences.
    // This can save two or four subtractions in many cases
    static inline return_type apply(segment_type1 const& a, segment_type2 const& b,
            coordinate_type const& dx_a, coordinate_type const& dy_a,
            coordinate_type const& dx_b, coordinate_type const& dy_b)
    {
        // 1) Handle "disjoint", probably common case.
        // per dimension, 2 cases: a_1----------a_2    b_1-------b_2 or B left of A
        coordinate_type ax_1, ax_2, bx_1, bx_2;
        bool ax_swapped = false, bx_swapped = false;
        detail::segment_arrange<segment_type1, 0>::apply(a, ax_1, ax_2, ax_swapped);
        detail::segment_arrange<segment_type2, 0>::apply(b, bx_1, bx_2, bx_swapped);
        if (ax_2 < bx_1 || ax_1 > bx_2)
        {
            return Policy::disjoint();
        }

        // 1b) In Y-dimension
        coordinate_type ay_1, ay_2, by_1, by_2;
        bool ay_swapped = false, by_swapped = false;
        detail::segment_arrange<segment_type1, 1>::apply(a, ay_1, ay_2, ay_swapped);
        detail::segment_arrange<segment_type2, 1>::apply(b, by_1, by_2, by_swapped);
        if (ay_2 < ay_1 || ay_1 > by_2)
        {
            return Policy::disjoint();
        }

        typedef side::side_by_triangle<coordinate_type> side;
        side_info sides;

        // 2) Calculate sides
        // Note: Do NOT yet calculate the determinant here, but use the SIDE strategy.
        // Determinant calculation is not robust; side (orient) can be made robust
        // (and is much robuster even without measures)
        sides.set<1>(side::apply(a.first, a.second, b.first),
                side::apply(a.first, a.second, b.second));

        if (sides.same<1>())
        {
            // Both points are at same side of other segment, we can leave
            return Policy::disjoint();
        }

        // 2b) For other segment
        sides.set<0>(side::apply(b.first, b.second, a.first),
                side::apply(b.first, b.second, a.second));

        if (sides.same<0>())
        {
            return Policy::disjoint();
        }

        // Degenerate cases: segments of single point, lying on other segment, non disjoint
        coordinate_type const zero = 0;
        if (math::equals(dx_a, zero) && math::equals(dy_a, zero))
        {
            return Policy::degenerate(a, true);
        }
        if (math::equals(dx_b, zero) && math::equals(dy_b, zero))
        {
            return Policy::degenerate(b, false);
        }

        bool collinear = sides.collinear();

        // Get the same type, but at least a double (also used for divisions
        typedef typename select_most_precise
            <
                coordinate_type, double
            >::type promoted_type;


        promoted_type const d = (dy_b * dx_a) - (dx_b * dy_a);
        // Determinant d should be nonzero.
        // If it is zero, we have an robustness issue here,
        // (and besides that we cannot divide by it)
        if(math::equals(d, zero) && ! collinear)
        //if(! collinear && sides.as_collinear())
        {
#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
            std::cout << "Determinant zero? Type : "
                << typeid(coordinate_type).name()
                << std::endl;

            std::cout << " dx_a : " << dx_a << std::endl;
            std::cout << " dy_a : " << dy_a << std::endl;
            std::cout << " dx_b : " << dx_b << std::endl;
            std::cout << " dy_b : " << dy_b << std::endl;

            std::cout << " side a <-> b.first : " << sides.get<0,0>() << std::endl;
            std::cout << " side a <-> b.second: " << sides.get<0,1>() << std::endl;
            std::cout << " side b <-> a.first : " << sides.get<1,0>() << std::endl;
            std::cout << " side b <-> a.second: " << sides.get<1,1>() << std::endl;
#endif

            if (sides.as_collinear())
            {
                sides.set<0>(0,0);
                sides.set<1>(0,0);
                collinear = true;
            }
            else
            {
                return Policy::error("Determinant zero!");
            }
        }

        if(collinear)
        {
            // Segments are collinear. We'll find out how.
            if (math::equals(dx_b, zero))
            {
                // Vertical -> Check y-direction
                return relate_collinear(a, b,
                        ay_1, ay_2, by_1, by_2,
                        ay_swapped, by_swapped);
            }
            else
            {
                // Check x-direction
                return relate_collinear(a, b,
                        ax_1, ax_2, bx_1, bx_2,
                        ax_swapped, bx_swapped);
            }
        }

        return Policy::segments_intersect(sides,
            dx_a, dy_a, dx_b, dy_b,
            a, b);
    }

private :

    /// Relate segments known collinear
    static inline return_type relate_collinear(segment_type1 const& a
            , segment_type2 const& b
            , coordinate_type a_1, coordinate_type a_2
            , coordinate_type b_1, coordinate_type b_2
            , bool a_swapped, bool b_swapped)
    {
        // All ca. 150 lines are about collinear rays
        // The intersections, if any, are always boundary points of the segments. No need to calculate anything.
        // However we want to find out HOW they intersect, there are many cases.
        // Most sources only provide the intersection (above) or that there is a collinearity (but not the points)
        // or some spare sources give the intersection points (calculated) but not how they align.
        // This source tries to give everything and still be efficient.
        // It is therefore (and because of the extensive clarification comments) rather long...

        // \see http://mpa.itc.it/radim/g50history/CMP/4.2.1-CERL-beta-libes/file475.txt
        // \see http://docs.codehaus.org/display/GEOTDOC/Point+Set+Theory+and+the+DE-9IM+Matrix
        // \see http://mathworld.wolfram.com/Line-LineIntersection.html

        // Because of collinearity the case is now one-dimensional and can be checked using intervals
        // This function is called either horizontally or vertically
        // We get then two intervals:
        // a_1-------------a_2 where a_1 < a_2
        // b_1-------------b_2 where b_1 < b_2
        // In all figures below a_1/a_2 denotes arranged intervals, a1-a2 or a2-a1 are still unarranged

        // Handle "equal", in polygon neighbourhood comparisons a common case

        // Check if segments are equal...
        bool const a1_eq_b1 = math::equals(get<0, 0>(a), get<0, 0>(b))
                    && math::equals(get<0, 1>(a), get<0, 1>(b));
        bool const a2_eq_b2 = math::equals(get<1, 0>(a), get<1, 0>(b))
                    && math::equals(get<1, 1>(a), get<1, 1>(b));
        if (a1_eq_b1 && a2_eq_b2)
        {
            return Policy::segment_equal(a, false);
        }

        // ... or opposite equal
        bool const a1_eq_b2 = math::equals(get<0, 0>(a), get<1, 0>(b))
                    && math::equals(get<0, 1>(a), get<1, 1>(b));
        bool const a2_eq_b1 = math::equals(get<1, 0>(a), get<0, 0>(b))
                    && math::equals(get<1, 1>(a), get<0, 1>(b));
        if (a1_eq_b2 && a2_eq_b1)
        {
            return Policy::segment_equal(a, true);
        }


        // The rest below will return one or two intersections.
        // The delegated class can decide which is the intersection point, or two, build the Intersection Matrix (IM)
        // For IM it is important to know which relates to which. So this information is given,
        // without performance penalties to intersection calculation

        bool const has_common_points = a1_eq_b1 || a1_eq_b2 || a2_eq_b1 || a2_eq_b2;


        // "Touch" -> one intersection point -> one but not two common points
        // -------->             A (or B)
        //         <----------   B (or A)
        //        a_2==b_1         (b_2==a_1 or a_2==b1)

        // The check a_2/b_1 is necessary because it excludes cases like
        // ------->
        //     --->
        // ... which are handled lateron

        // Corresponds to 4 cases, of which the equal points are determined above
        // #1: a1---->a2 b1--->b2   (a arrives at b's border)
        // #2: a2<----a1 b2<---b1   (b arrives at a's border)
        // #3: a1---->a2 b2<---b1   (both arrive at each others border)
        // #4: a2<----a1 b1--->b2   (no arrival at all)
        // Where the arranged forms have two forms:
        //    a_1-----a_2/b_1-------b_2 or reverse (B left of A)
        if (has_common_points && (math::equals(a_2, b_1) || math::equals(b_2, a_1)))
        {
            if (a2_eq_b1) return Policy::collinear_touch(get<1, 0>(a), get<1, 1>(a), 0, -1);
            if (a1_eq_b2) return Policy::collinear_touch(get<0, 0>(a), get<0, 1>(a), -1, 0);
            if (a2_eq_b2) return Policy::collinear_touch(get<1, 0>(a), get<1, 1>(a), 0, 0);
            if (a1_eq_b1) return Policy::collinear_touch(get<0, 0>(a), get<0, 1>(a), -1, -1);
        }


        // "Touch/within" -> there are common points and also an intersection of interiors:
        // Corresponds to many cases:
        // #1a: a1------->a2  #1b:        a1-->a2
        //          b1--->b2         b1------->b2
        // #2a: a2<-------a1  #2b:        a2<--a1
        //          b1--->b2         b1------->b2
        // #3a: a1------->a2  #3b:        a1-->a2
        //          b2<---b1         b2<-------b1
        // #4a: a2<-------a1  #4b:        a2<--a1
        //          b2<---b1         b2<-------b1

        // Note: next cases are similar and handled by the code
        // #4c: a1--->a2
        //      b1-------->b2
        // #4d: a1-------->a2
        //      b1-->b2

        // For case 1-4: a_1 < (b_1 or b_2) < a_2, two intersections are equal to segment B
        // For case 5-8: b_1 < (a_1 or a_2) < b_2, two intersections are equal to segment A
        if (has_common_points)
        {
            // Either A is in B, or B is in A, or (in case of robustness/equals)
            // both are true, see below
            bool a_in_b = (b_1 < a_1 && a_1 < b_2) || (b_1 < a_2 && a_2 < b_2);
            bool b_in_a = (a_1 < b_1 && b_1 < a_2) || (a_1 < b_2 && b_2 < a_2);

            if (a_in_b && b_in_a)
            {
                // testcase "ggl_list_20110306_javier"
                // In robustness it can occur that a point of A is inside B AND a point of B is inside A,
                // still while has_common_points is true (so one point equals the other).
                // If that is the case we select on length.
                coordinate_type const length_a = abs(a_1 - a_2);
                coordinate_type const length_b = abs(b_1 - b_2);
                if (length_a > length_b)
                {
                    a_in_b = false;
                }
                else
                {
                    b_in_a = false;
                }
            }

            int const arrival_a = a_in_b ? 1 : -1;
            if (a2_eq_b2) return Policy::collinear_interior_boundary_intersect(a_in_b ? a : b, a_in_b, 0, 0, false);
            if (a1_eq_b2) return Policy::collinear_interior_boundary_intersect(a_in_b ? a : b, a_in_b, arrival_a, 0, true);
            if (a2_eq_b1) return Policy::collinear_interior_boundary_intersect(a_in_b ? a : b, a_in_b, 0, -arrival_a, true);
            if (a1_eq_b1) return Policy::collinear_interior_boundary_intersect(a_in_b ? a : b, a_in_b, arrival_a, -arrival_a, false);
        }

        bool const opposite = a_swapped ^ b_swapped;


        // "Inside", a completely within b or b completely within a
        // 2 cases:
        // case 1:
        //        a_1---a_2        -> take A's points as intersection points
        //   b_1------------b_2
        // case 2:
        //   a_1------------a_2
        //       b_1---b_2         -> take B's points
        if (a_1 > b_1 && a_2 < b_2)
        {
            // A within B
            return Policy::collinear_a_in_b(a, opposite);
        }
        if (b_1 > a_1 && b_2 < a_2)
        {
            // B within A
            return Policy::collinear_b_in_a(b, opposite);
        }


        /*

        Now that all cases with equal,touch,inside,disjoint,
        degenerate are handled the only thing left is an overlap

        Either a1 is between b1,b2
        or a2 is between b1,b2 (a2 arrives)

        Next table gives an overview.
        The IP's are ordered following the line A1->A2

             |                                 |
             |          a_2 in between         |       a_1 in between
             |                                 |
        -----+---------------------------------+--------------------------
             |   a1--------->a2                |       a1--------->a2
             |          b1----->b2             |   b1----->b2
             |   (b1,a2), a arrives            |   (a1,b2), b arrives
             |                                 |
        -----+---------------------------------+--------------------------
        a sw.|   a2<---------a1*               |       a2<---------a1*
             |           b1----->b2            |   b1----->b2
             |   (a1,b1), no arrival           |   (b2,a2), a and b arrive
             |                                 |
        -----+---------------------------------+--------------------------
             |   a1--------->a2                |       a1--------->a2
        b sw.|           b2<-----b1            |   b2<-----b1
             |   (b2,a2), a and b arrive       |   (a1,b1), no arrival
             |                                 |
        -----+---------------------------------+--------------------------
        a sw.|    a2<---------a1*              |       a2<---------a1*
        b sw.|            b2<-----b1           |   b2<-----b1
             |   (a1,b2), b arrives            |   (b1,a2), a arrives
             |                                 |
        -----+---------------------------------+--------------------------
        * Note that a_1 < a_2, and a1 <> a_1; if a is swapped,
          the picture might seem wrong but it (supposed to be) is right.
        */

        bool const both_swapped = a_swapped && b_swapped;
        if (b_1 < a_2 && a_2 < b_2)
        {
            // Left column, from bottom to top
            return
                both_swapped ? Policy::collinear_overlaps(get<0, 0>(a), get<0, 1>(a), get<1, 0>(b), get<1, 1>(b), -1,  1, opposite)
                : b_swapped  ? Policy::collinear_overlaps(get<1, 0>(b), get<1, 1>(b), get<1, 0>(a), get<1, 1>(a),  1,  1, opposite)
                : a_swapped  ? Policy::collinear_overlaps(get<0, 0>(a), get<0, 1>(a), get<0, 0>(b), get<0, 1>(b), -1, -1, opposite)
                :              Policy::collinear_overlaps(get<0, 0>(b), get<0, 1>(b), get<1, 0>(a), get<1, 1>(a),  1, -1, opposite)
                ;
        }
        if (b_1 < a_1 && a_1 < b_2)
        {
            // Right column, from bottom to top
            return
                both_swapped ? Policy::collinear_overlaps(get<0, 0>(b), get<0, 1>(b), get<1, 0>(a), get<1, 1>(a),  1, -1, opposite)
                : b_swapped  ? Policy::collinear_overlaps(get<0, 0>(a), get<0, 1>(a), get<0, 0>(b), get<0, 1>(b), -1, -1, opposite)
                : a_swapped  ? Policy::collinear_overlaps(get<1, 0>(b), get<1, 1>(b), get<1, 0>(a), get<1, 1>(a),  1,  1, opposite)
                :              Policy::collinear_overlaps(get<0, 0>(a), get<0, 1>(a), get<1, 0>(b), get<1, 1>(b), -1,  1, opposite)
                ;
        }
        // Nothing should goes through. If any we have made an error
        // Robustness: it can occur here...
        return Policy::error("Robustness issue, non-logical behaviour");
    }
};


}} // namespace strategy::intersection



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_INTERSECTION_HPP
