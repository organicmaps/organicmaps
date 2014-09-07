// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013, 2014.
// Modifications copyright (c) 2013, 2014 Oracle and/or its affiliates.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#ifndef BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP
#define BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP


#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_calculation_type.hpp>

#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace within
{


// Fix for https://svn.boost.org/trac/boost/ticket/9628
// For floating point coordinates, the <1> coordinate of a point is compared
// with the segment's points using some EPS. If the coordinates are "equal"
// the sides are calculated. Therefore we can treat a segment as a long areal
// geometry having some width. There is a small ~triangular area somewhere
// between the segment's effective area and a segment's line used in sides
// calculation where the segment is on the one side of the line but on the
// other side of a segment (due to the width).
// For the s1 of a segment going NE the real side is RIGHT but the point may
// be detected as LEFT, like this:
//                     RIGHT
//                 ___----->
//                  ^      O Pt  __ __
//                 EPS     __ __
//                  v__ __ BUT DETECTED AS LEFT OF THIS LINE
//             _____7
//       _____/
// _____/
template <typename CSTag>
struct winding_side_equal
{
    typedef typename strategy::side::services::default_strategy
        <
            CSTag
        >::type strategy_side_type;

    template <size_t D, typename Point, typename PointOfSegment>
    static inline int apply(Point const& point,
                            PointOfSegment const& se,
                            int count)
    {
        // Create a vertical segment intersecting the original segment's endpoint
        // equal to the point, with the derived direction (UP/DOWN).
        // Set only the 2 first coordinates, the other ones are ignored
        PointOfSegment ss1, ss2;
        set<1-D>(ss1, get<1-D>(se));
        set<1-D>(ss2, get<1-D>(se));
        if ( count > 0 ) // UP
        {
            set<D>(ss1, 0);
            set<D>(ss2, 1);
        }
        else // DOWN
        {
            set<D>(ss1, 1);
            set<D>(ss2, 0);
        }
        // Check the side using this vertical segment
        return strategy_side_type::apply(ss1, ss2, point);
    }
};

// The optimization for cartesian
template <>
struct winding_side_equal<cartesian_tag>
{
    template <size_t D, typename Point, typename PointOfSegment>
    static inline int apply(Point const& point,
                            PointOfSegment const& se,
                            int count)
    {
        return math::equals(get<1-D>(point), get<1-D>(se)) ?
                0 :
                get<1-D>(point) < get<1-D>(se) ?
                    // assuming count is equal to 1 or -1
                    count : // ( count > 0 ? 1 : -1) :
                    -count; // ( count > 0 ? -1 : 1) ;
    }
};


/*!
\brief Within detection using winding rule
\ingroup strategies
\tparam Point \tparam_point
\tparam PointOfSegment \tparam_segment_point
\tparam CalculationType \tparam_calculation
\author Barend Gehrels
\note The implementation is inspired by terralib http://www.terralib.org (LGPL)
\note but totally revised afterwards, especially for cases on segments
\note Only dependant on "side", -> agnostic, suitable for spherical/latlong

\qbk{
[heading See also]
[link geometry.reference.algorithms.within.within_3_with_strategy within (with strategy)]
}
 */
template
<
    typename Point,
    typename PointOfSegment = Point,
    typename CalculationType = void
>
class winding
{
    typedef typename select_calculation_type
        <
            Point,
            PointOfSegment,
            CalculationType
        >::type calculation_type;


    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<Point>::type
        >::type strategy_side_type;


    /*! subclass to keep state */
    class counter
    {
        int m_count;
        bool m_touches;

        inline int code() const
        {
            return m_touches ? 0 : m_count == 0 ? -1 : 1;
        }

    public :
        friend class winding;

        inline counter()
            : m_count(0)
            , m_touches(false)
        {}

    };


    template <size_t D>
    static inline int check_touch(Point const& point,
                PointOfSegment const& seg1, PointOfSegment const& seg2,
                counter& state)
    {
        calculation_type const p = get<D>(point);
        calculation_type const s1 = get<D>(seg1);
        calculation_type const s2 = get<D>(seg2);
        if ((s1 <= p && s2 >= p) || (s2 <= p && s1 >= p))
        {
            state.m_touches = true;
        }
        return 0;
    }


    template <size_t D>
    static inline int check_segment(Point const& point,
                PointOfSegment const& seg1, PointOfSegment const& seg2,
                counter& state, bool& eq1, bool& eq2)
    {
        calculation_type const p = get<D>(point);
        calculation_type const s1 = get<D>(seg1);
        calculation_type const s2 = get<D>(seg2);

        // Check if one of segment endpoints is at same level of point
        eq1 = math::equals(s1, p);
        eq2 = math::equals(s2, p);

        if (eq1 && eq2)
        {
            // Both equal p -> segment is horizontal (or vertical for D=0)
            // The only thing which has to be done is check if point is ON segment
            return check_touch<1 - D>(point, seg1, seg2, state);
        }

        return
              eq1 ? (s2 > p ?  1 : -1)  // Point on level s1, UP/DOWN depending on s2
            : eq2 ? (s1 > p ? -1 :  1)  // idem
            : s1 < p && s2 > p ?  2     // Point between s1 -> s2 --> UP
            : s2 < p && s1 > p ? -2     // Point between s2 -> s1 --> DOWN
            : 0;
    }


public :

    // Typedefs and static methods to fulfill the concept
    typedef Point point_type;
    typedef PointOfSegment segment_point_type;
    typedef counter state_type;

    static inline bool apply(Point const& point,
                PointOfSegment const& s1, PointOfSegment const& s2,
                counter& state)
    {
        bool eq1 = false;
        bool eq2 = false;
        boost::ignore_unused(eq2);

        int count = check_segment<1>(point, s1, s2, state, eq1, eq2);
        if (count != 0)
        {
            int side = 0;
            if ( count == 1 || count == -1 )
            {
                side = winding_side_equal<typename cs_tag<Point>::type>
                            ::template apply<1>(point, eq1 ? s1 : s2, count);
            }
            else
            {
                side = strategy_side_type::apply(s1, s2, point);
            }
            
            if (side == 0)
            {
                // Point is lying on segment
                state.m_touches = true;
                state.m_count = 0;
                return false;
            }

            // Side is NEG for right, POS for left.
            // The count is -2 for down, 2 for up (or -1/1)
            // Side positive thus means UP and LEFTSIDE or DOWN and RIGHTSIDE
            // See accompagnying figure (TODO)
            if (side * count > 0)
            {
                state.m_count += count;
            }
        }
        return ! state.m_touches;
    }

    static inline int result(counter const& state)
    {
        return state.code();
    }
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

// Register using "areal_tag" for ring, polygon, multi-polygon
template <typename AnyTag, typename Point, typename Geometry>
struct default_strategy<point_tag, AnyTag, point_tag, areal_tag, cartesian_tag, cartesian_tag, Point, Geometry>
{
    typedef winding<Point, typename geometry::point_type<Geometry>::type> type;
};

template <typename AnyTag, typename Point, typename Geometry>
struct default_strategy<point_tag, AnyTag, point_tag, areal_tag, spherical_tag, spherical_tag, Point, Geometry>
{
    typedef winding<Point, typename geometry::point_type<Geometry>::type> type;
};

// TODO: use linear_tag and pointlike_tag the same way how areal_tag is used

template <typename Point, typename Geometry, typename AnyTag>
struct default_strategy<point_tag, AnyTag, point_tag, AnyTag, cartesian_tag, cartesian_tag, Point, Geometry>
{
    typedef winding<Point, typename geometry::point_type<Geometry>::type> type;
};

template <typename Point, typename Geometry, typename AnyTag>
struct default_strategy<point_tag, AnyTag, point_tag, AnyTag, spherical_tag, spherical_tag, Point, Geometry>
{
    typedef winding<Point, typename geometry::point_type<Geometry>::type> type;
};

} // namespace services

#endif


}} // namespace strategy::within



#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace strategy { namespace covered_by { namespace services
{

// Register using "areal_tag" for ring, polygon, multi-polygon
template <typename AnyTag, typename Point, typename Geometry>
struct default_strategy<point_tag, AnyTag, point_tag, areal_tag, cartesian_tag, cartesian_tag, Point, Geometry>
{
    typedef strategy::within::winding<Point, typename geometry::point_type<Geometry>::type> type;
};

template <typename AnyTag, typename Point, typename Geometry>
struct default_strategy<point_tag, AnyTag, point_tag, areal_tag, spherical_tag, spherical_tag, Point, Geometry>
{
    typedef strategy::within::winding<Point, typename geometry::point_type<Geometry>::type> type;
};

// TODO: use linear_tag and pointlike_tag the same way how areal_tag is used

template <typename Point, typename Geometry, typename AnyTag>
struct default_strategy<point_tag, AnyTag, point_tag, AnyTag, cartesian_tag, cartesian_tag, Point, Geometry>
{
    typedef strategy::within::winding<Point, typename geometry::point_type<Geometry>::type> type;
};

template <typename Point, typename Geometry, typename AnyTag>
struct default_strategy<point_tag, AnyTag, point_tag, AnyTag, spherical_tag, spherical_tag, Point, Geometry>
{
    typedef strategy::within::winding<Point, typename geometry::point_type<Geometry>::type> type;
};

}}} // namespace strategy::covered_by::services
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGY_AGNOSTIC_POINT_IN_POLY_WINDING_HPP
