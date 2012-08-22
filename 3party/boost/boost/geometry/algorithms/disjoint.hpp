// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DISJOINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DISJOINT_HPP

#include <cstddef>
#include <deque>

#include <boost/mpl/if.hpp>
#include <boost/range.hpp>

#include <boost/static_assert.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>

#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{

template<typename Geometry>
struct check_each_ring_for_within
{
    bool has_within;
    Geometry const& m_geometry;

    inline check_each_ring_for_within(Geometry const& g)
        : has_within(false)
        , m_geometry(g)
    {}

    template <typename Range>
    inline void apply(Range const& range)
    {
        typename geometry::point_type<Range>::type p;
        geometry::point_on_border(p, range);
        if (geometry::within(p, m_geometry))
        {
            has_within = true;
        }
    }
};

template <typename FirstGeometry, typename SecondGeometry>
inline bool rings_containing(FirstGeometry const& geometry1,
                SecondGeometry const& geometry2)
{
    check_each_ring_for_within<FirstGeometry> checker(geometry1);
    geometry::detail::for_each_range(geometry2, checker);
    return checker.has_within;
}


struct assign_disjoint_policy
{
    // We want to include all points:
    static bool const include_no_turn = true;
    static bool const include_degenerate = true;
    static bool const include_opposite = true;

    // We don't assign extra info:
    template 
	<
		typename Info,
		typename Point1,
		typename Point2,
		typename IntersectionInfo,
		typename DirInfo
	>
    static inline void apply(Info& , Point1 const& , Point2 const&,
                IntersectionInfo const&, DirInfo const&)
    {}
};
   

template <typename Geometry1, typename Geometry2>
struct disjoint_linear
{
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        typedef typename geometry::point_type<Geometry1>::type point_type;

        typedef overlay::turn_info<point_type> turn_info;
        std::deque<turn_info> turns;

        // Specify two policies:
        // 1) Stop at any intersection
        // 2) In assignment, include also degenerate points (which are normally skipped)
        disjoint_interrupt_policy policy;
        geometry::get_turns
            <
                false, false, 
                assign_disjoint_policy
            >(geometry1, geometry2, turns, policy);
        if (policy.has_intersections)
        {
            return false;
        }

        return true;
    }
};

template <typename Segment1, typename Segment2>
struct disjoint_segment
{
    static inline bool apply(Segment1 const& segment1, Segment2 const& segment2)
    {
        typedef typename point_type<Segment1>::type point_type;

        segment_intersection_points<point_type> is
            = strategy::intersection::relate_cartesian_segments
            <
                policies::relate::segments_intersection_points
                    <
                        Segment1,
                        Segment2,
                        segment_intersection_points<point_type>
                    >
            >::apply(segment1, segment2);

        return is.count == 0;
    }
};

template <typename Geometry1, typename Geometry2>
struct general_areal
{
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        if (! disjoint_linear<Geometry1, Geometry2>::apply(geometry1, geometry2))
        {
            return false;
        }

        // If there is no intersection of segments, they might located
        // inside each other
        if (rings_containing(geometry1, geometry2)
            || rings_containing(geometry2, geometry1))
        {
            return false;
        }

        return true;
    }
};


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    std::size_t DimensionCount
>
struct disjoint
    : detail::disjoint::general_areal<Geometry1, Geometry2>
{};


template <typename Point1, typename Point2, std::size_t DimensionCount>
struct disjoint<point_tag, point_tag, Point1, Point2, DimensionCount>
    : detail::disjoint::point_point<Point1, Point2, 0, DimensionCount>
{};


template <typename Box1, typename Box2, std::size_t DimensionCount>
struct disjoint<box_tag, box_tag, Box1, Box2, DimensionCount>
    : detail::disjoint::box_box<Box1, Box2, 0, DimensionCount>
{};


template <typename Point, typename Box, std::size_t DimensionCount>
struct disjoint<point_tag, box_tag, Point, Box, DimensionCount>
    : detail::disjoint::point_box<Point, Box, 0, DimensionCount>
{};

template <typename Linestring1, typename Linestring2>
struct disjoint<linestring_tag, linestring_tag, Linestring1, Linestring2, 2>
    : detail::disjoint::disjoint_linear<Linestring1, Linestring2>
{};

template <typename Linestring1, typename Linestring2>
struct disjoint<segment_tag, segment_tag, Linestring1, Linestring2, 2>
    : detail::disjoint::disjoint_segment<Linestring1, Linestring2>
{};

template <typename Linestring, typename Segment>
struct disjoint<linestring_tag, segment_tag, Linestring, Segment, 2>
    : detail::disjoint::disjoint_linear<Linestring, Segment>
{};


template
<
    typename GeometryTag1, typename GeometryTag2,
    typename Geometry1, typename Geometry2,
    std::size_t DimensionCount
>
struct disjoint_reversed
{
    static inline bool apply(Geometry1 const& g1, Geometry2 const& g2)
    {
        return disjoint
            <
                GeometryTag2, GeometryTag1,
                Geometry2, Geometry1,
                DimensionCount
            >::apply(g2, g1);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



/*!
\brief \brief_check2{are disjoint}
\ingroup disjoint
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{are disjoint}

\qbk{[include reference/algorithms/disjoint.qbk]}
*/
template <typename Geometry1, typename Geometry2>
inline bool disjoint(Geometry1 const& geometry1,
            Geometry2 const& geometry2)
{
    concept::check_concepts_and_equal_dimensions
        <
            Geometry1 const,
            Geometry2 const
        >();

    return boost::mpl::if_c
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::disjoint_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                dimension<Geometry1>::type::value
            >,
            dispatch::disjoint
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                dimension<Geometry1>::type::value
            >
        >::type::apply(geometry1, geometry2);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DISJOINT_HPP
