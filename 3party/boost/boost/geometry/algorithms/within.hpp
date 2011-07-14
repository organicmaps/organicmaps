// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP
#define BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP


#include <cstddef>

#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/algorithms/make.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/interior_rings.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/within.hpp>
#include <boost/geometry/strategies/concepts/within_concept.hpp>
#include <boost/geometry/util/order_as_direction.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within
{


/*!
    \brief Implementation for boxes
    \ingroup boolean_relations
    \note Should have strategy for e.g. Wrangel
 */
template
<
    typename Point,
    typename Box,
    typename Strategy,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct point_in_box
{
    static inline int apply(Point const& p, Box const& b, Strategy const& s)
    {
        assert_dimension_equal<Point, Box>();

        if (get<Dimension>(p) <= get<min_corner, Dimension>(b)
            || get<Dimension>(p) >= get<max_corner, Dimension>(b))
        {
            return -1;
        }

        return point_in_box
            <
                Point,
                Box,
                Strategy,
                Dimension + 1,
                DimensionCount
            >::apply(p, b, s);
    }
};

template
<
    typename Point,
    typename Box,
    typename Strategy,
    std::size_t DimensionCount
>
struct point_in_box<Point, Box, Strategy, DimensionCount, DimensionCount>
{
    static inline int apply(Point const& , Box const& , Strategy const& )
    {
        return 1;
    }
};


template
<
    typename Box1,
    typename Box2,
    typename Strategy,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct box_in_box
{
    static inline int apply(Box1 const& b1, Box2 const& b2, Strategy const& s)
    {
        assert_dimension_equal<Box1, Box2>();

        if (get<min_corner, Dimension>(b1) <= get<min_corner, Dimension>(b2)
            || get<max_corner, Dimension>(b1) >= get<max_corner, Dimension>(b2))
        {
            return -1;
        }

        return box_in_box
            <
                Box1,
                Box2,
                Strategy,
                Dimension + 1,
                DimensionCount
            >::apply(b1, b2, s);
    }
};

template
<
    typename Box1,
    typename Box2,
    typename Strategy,
    std::size_t DimensionCount
>
struct box_in_box<Box1, Box2, Strategy, DimensionCount, DimensionCount>
{
    static inline int apply(Box1 const& , Box2 const& , Strategy const&)
    {
        return 1;
    }
};


template
<
    typename Point,
    typename Ring,
    iterate_direction Direction,
    closure_selector Closure,
    typename Strategy
>
struct point_in_ring
{
    BOOST_CONCEPT_ASSERT( (geometry::concept::WithinStrategy<Strategy>) );

    static inline int apply(Point const& point, Ring const& ring,
            Strategy const& strategy)
    {
        if (boost::size(ring)
                < core_detail::closure::minimum_ring_size<Closure>::value)
        {
            return -1;
        }

        typedef typename reversible_view<Ring const, Direction>::type rev_view_type;
        typedef typename closeable_view
            <
                rev_view_type const, Closure
            >::type cl_view_type;
        typedef typename boost::range_iterator<cl_view_type const>::type iterator_type;

        rev_view_type rev_view(ring);
        cl_view_type view(rev_view);
        typename Strategy::state_type state;
        iterator_type it = boost::begin(view);
        iterator_type end = boost::end(view);

        for (iterator_type previous = it++;
            it != end;
            ++previous, ++it)
        {
            if (! strategy.apply(point, *previous, *it, state))
            {
                return false;
            }
        }

        return strategy.result(state);
    }
};



// Polygon: in exterior ring, and if so, not within interior ring(s)
template
<
    typename Point,
    typename Polygon,
    iterate_direction Direction,
    closure_selector Closure,
    typename Strategy
>
struct point_in_polygon
{
    BOOST_CONCEPT_ASSERT( (geometry::concept::WithinStrategy<Strategy>) );

    static inline int apply(Point const& point, Polygon const& poly,
            Strategy const& strategy)
    {
        int const code = point_in_ring
            <
                Point,
                typename ring_type<Polygon>::type,
                Direction,
                Closure,
                Strategy
            >::apply(point, exterior_ring(poly), strategy);

        if (code == 1)
        {
            typename interior_return_type<Polygon const>::type rings
                        = interior_rings(poly);
            for (BOOST_AUTO_TPL(it, boost::begin(rings));
                it != boost::end(rings);
                ++it)
            {
                int const interior_code = point_in_ring
                    <
                        Point,
                        typename ring_type<Polygon>::type,
                        Direction,
                        Closure,
                        Strategy
                    >::apply(point, *it, strategy);

                if (interior_code != -1)
                {
                    // If 0, return 0 (touch)
                    // If 1 (inside hole) return -1 (outside polygon)
                    // If -1 (outside hole) check other holes if any
                    return -interior_code;
                }
            }
        }
        return code;
    }
};

}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag1,
    typename Tag2,
    typename Geometry1,
    typename Geometry2,
    typename Strategy
>
struct within
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry1, Geometry2>)
        );
};


template <typename Point, typename Box, typename Strategy>
struct within<point_tag, box_tag, Point, Box, Strategy>
    : detail::within::point_in_box
            <
                Point,
                Box,
                Strategy,
                0,
                dimension<Point>::type::value
            >
{};

template <typename Box1, typename Box2, typename Strategy>
struct within<box_tag, box_tag, Box1, Box2, Strategy>
    : detail::within::box_in_box
            <
                Box1,
                Box2,
                Strategy,
                0,
                dimension<Box1>::type::value
            >
{};



template <typename Point, typename Ring, typename Strategy>
struct within<point_tag, ring_tag, Point, Ring, Strategy>
    : detail::within::point_in_ring
        <
            Point,
            Ring,
            order_as_direction<geometry::point_order<Ring>::value>::value,
            geometry::closure<Ring>::value,
            Strategy
        >
{};

template <typename Point, typename Polygon, typename Strategy>
struct within<point_tag, polygon_tag, Point, Polygon, Strategy>
    : detail::within::point_in_polygon
        <
            Point,
            Polygon,
            order_as_direction<geometry::point_order<Polygon>::value>::value,
            geometry::closure<Polygon>::value,
            Strategy
        >
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace strategy { namespace within
{

/// Strategy for box-in-box (not used but has to be present for default strategy)
struct unused_strategy {};

namespace services
{

// Specialize for box-in-areal (box-in-box). This is meant to do box-in-box
// but will be catched by box-in-any-areal, which has to change later
// (we might introduce another tag which is not "areal", derived by poly/ring/
// multi_poly, but NOT by box, and use that here. E.g. "polygonal")
// Using cartesian prevents spherical yet from compiling, which is good.
template <typename Box1, typename Box2>
struct default_strategy<box_tag, areal_tag, cartesian_tag, cartesian_tag, Box1, Box2>
{
    typedef unused_strategy type;
};

} // namespace services

}} // namespace strategy::within

#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

/*!
\brief \brief_check12{is completely inside}
\ingroup within
\details \details_check12{within, is completely inside}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry1 geometry which might be within the second geometry
\param geometry2 geometry which might contain the first geometry
\return true if geometry1 is completely contained within geometry2,
    else false
\note The default strategy is used for within detection


\qbk{[include reference/algorithms/within.qbk]}

\qbk{
[heading Example]
[within]
[within_output]
}
 */
template<typename Geometry1, typename Geometry2>
inline bool within(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    typedef typename point_type<Geometry1>::type point_type1;
    typedef typename point_type<Geometry2>::type point_type2;

    typedef typename strategy::within::services::default_strategy
        <
            typename tag<Geometry1>::type,
            typename tag_cast<typename tag<Geometry2>::type, areal_tag>::type,
            typename cs_tag<point_type1>::type,
            typename cs_tag<point_type2>::type,
            point_type1,
            point_type2
        >::type strategy_type;

    return dispatch::within
        <
            typename tag<Geometry1>::type,
            typename tag<Geometry2>::type,
            Geometry1,
            Geometry2,
            strategy_type
        >::apply(geometry1, geometry2, strategy_type()) == 1;
}

/*!
\brief \brief_check12{is completely inside} \brief_strategy
\ingroup within
\details \details_check12{within, is completely inside}, \brief_strategy. \details_strategy_reasons
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry1 \param_geometry geometry which might be within the second geometry
\param geometry2 \param_geometry which might contain the first geometry
\param strategy strategy to be used
\return true if geometry1 is completely contained within geometry2,
    else false

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/within.qbk]}
\qbk{
[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_within_winding Winding (coordinate system agnostic)]
\* [link geometry.reference.strategies.strategy_within_franklin Franklin (cartesian)]
\* [link geometry.reference.strategies.strategy_within_crossings_multiply Crossings Multiply (cartesian)]

[heading Example]
[within_strategy]
[within_strategy_output]

}
*/
template<typename Geometry1, typename Geometry2, typename Strategy>
inline bool within(Geometry1 const& geometry1, Geometry2 const& geometry2,
        Strategy const& strategy)
{
    // Always assume a point-in-polygon strategy here.
    // Because for point-in-box, it makes no sense to specify one.
    BOOST_CONCEPT_ASSERT( (geometry::concept::WithinStrategy<Strategy>) );

    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    return dispatch::within
        <
            typename tag<Geometry1>::type,
            typename tag<Geometry2>::type,
            Geometry1,
            Geometry2,
            Strategy
        >::apply(geometry1, geometry2, strategy) == 1;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_WITHIN_HPP
