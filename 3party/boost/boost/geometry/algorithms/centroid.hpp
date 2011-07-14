// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP


#include <cstddef>

#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/centroid.hpp>
#include <boost/geometry/strategies/concepts/centroid_concept.hpp>
#include <boost/geometry/views/closeable_view.hpp>

#include <boost/geometry/util/for_each_coordinate.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>



namespace boost { namespace geometry
{


#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)

/*!
\brief Centroid Exception
\ingroup centroid
\details The centroid_exception is thrown if the free centroid function is called with
    geometries for which the centroid cannot be calculated. For example: a linestring
    without points, a polygon without points, an empty multi-geometry.
\qbk{
[heading See also]
\* [link geometry.reference.algorithms.centroid the centroid function]
}

 */
class centroid_exception : public geometry::exception
{
public:

    inline centroid_exception() {}

    virtual char const* what() const throw()
    {
        return "Boost.Geometry Centroid calculation exception";
    }
};

#endif


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace centroid
{

template<typename Point, typename PointCentroid, typename Strategy>
struct centroid_point
{
    static inline void apply(Point const& point, PointCentroid& centroid,
            Strategy const&)
    {
        geometry::convert(point, centroid);
    }
};

template
<
    typename Box,
    typename Point,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct centroid_box_calculator
{
    typedef typename select_coordinate_type
        <
            Box, Point
        >::type coordinate_type;
    static inline void apply(Box const& box, Point& centroid)
    {
        coordinate_type const c1 = get<min_corner, Dimension>(box);
        coordinate_type const c2 = get<max_corner, Dimension>(box);
        coordinate_type m = c1 + c2;
        m /= 2.0;

        set<Dimension>(centroid, m);

        centroid_box_calculator
            <
                Box, Point,
                Dimension + 1, DimensionCount
            >::apply(box, centroid);
    }
};


template<typename Box, typename Point, std::size_t DimensionCount>
struct centroid_box_calculator<Box, Point, DimensionCount, DimensionCount>
{
    static inline void apply(Box const& , Point& )
    {
    }
};


template<typename Box, typename Point, typename Strategy>
struct centroid_box
{
    static inline void apply(Box const& box, Point& centroid,
            Strategy const&)
    {
        centroid_box_calculator
            <
                Box, Point,
                0, dimension<Box>::type::value
            >::apply(box, centroid);
    }
};


// There is one thing where centroid is different from e.g. within.
// If the ring has only one point, it might make sense that
// that point is the centroid.
template<typename Point, typename Range>
inline bool range_ok(Range const& range, Point& centroid)
{
    std::size_t const n = boost::size(range);
    if (n > 1)
    {
        return true;
    }
    else if (n <= 0)
    {
#if ! defined(BOOST_GEOMETRY_CENTROID_NO_THROW)
        throw centroid_exception();
#endif
        return false;
    }
    else // if (n == 1)
    {
        // Take over the first point in a "coordinate neutral way"
        geometry::convert(*boost::begin(range), centroid);
        return false;
    }
    return true;
}


/*!
    \brief Calculate the centroid of a ring.
*/
template<typename Ring, closure_selector Closure, typename Strategy>
struct centroid_range_state
{
    static inline void apply(Ring const& ring,
            Strategy const& strategy, typename Strategy::state_type& state)
    {
        typedef typename closeable_view<Ring const, Closure>::type view_type;

        typedef typename boost::range_iterator<view_type const>::type iterator_type;

        view_type view(ring);
        iterator_type it = boost::begin(view);
        iterator_type end = boost::end(view);

        for (iterator_type previous = it++;
            it != end;
            ++previous, ++it)
        {
            Strategy::apply(*previous, *it, state);
        }
    }
};

template<typename Range, typename Point, closure_selector Closure, typename Strategy>
struct centroid_range
{
    static inline void apply(Range const& range, Point& centroid,
            Strategy const& strategy)
    {
        if (range_ok(range, centroid))
        {
            typename Strategy::state_type state;
            centroid_range_state
                <
                    Range,
                    Closure,
                    Strategy
                >::apply(range, strategy, state);
            Strategy::result(state, centroid);
        }
    }
};


/*!
    \brief Centroid of a polygon.
    \note Because outer ring is clockwise, inners are counter clockwise,
    triangle approach is OK and works for polygons with rings.
*/
template<typename Polygon, typename Strategy>
struct centroid_polygon_state
{
    typedef typename ring_type<Polygon>::type ring_type;

    static inline void apply(Polygon const& poly,
            Strategy const& strategy, typename Strategy::state_type& state)
    {
        typedef centroid_range_state
            <
                ring_type,
                geometry::closure<ring_type>::value,
                Strategy
            > per_ring;

        per_ring::apply(exterior_ring(poly), strategy, state);

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(poly);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            per_ring::apply(*it, strategy, state);
        }
    }
};

template<typename Polygon, typename Point, typename Strategy>
struct centroid_polygon
{
    static inline void apply(Polygon const& poly, Point& centroid,
            Strategy const& strategy)
    {
        if (range_ok(exterior_ring(poly), centroid))
        {
            typename Strategy::state_type state;
            centroid_polygon_state
                <
                    Polygon,
                    Strategy
                >::apply(poly, strategy, state);
            Strategy::result(state, centroid);
        }
    }
};


}} // namespace detail::centroid
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename Geometry,
    typename Point,
    typename Strategy
>
struct centroid {};

template
<
    typename Geometry,
    typename Point,
    typename Strategy
>
struct centroid<point_tag, Geometry, Point, Strategy>
    : detail::centroid::centroid_point<Geometry, Point, Strategy>
{};

template
<
    typename Box,
    typename Point,
    typename Strategy
>
struct centroid<box_tag, Box, Point, Strategy>
    : detail::centroid::centroid_box<Box, Point, Strategy>
{};

template <typename Ring, typename Point, typename Strategy>
struct centroid<ring_tag, Ring, Point, Strategy>
    : detail::centroid::centroid_range
        <
            Ring,
            Point,
            geometry::closure<Ring>::value,
            Strategy
        >
{};

template <typename Linestring, typename Point, typename Strategy>
struct centroid<linestring_tag, Linestring, Point, Strategy>
    : detail::centroid::centroid_range
        <
            Linestring,
            Point,
            closed,
            Strategy
        >
 {};

template <typename Polygon, typename Point, typename Strategy>
struct centroid<polygon_tag, Polygon, Point, Strategy>
    : detail::centroid::centroid_polygon<Polygon, Point, Strategy>
 {};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_calc{centroid} \brief_strategy
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_strategy_reasons
\tparam Geometry \tparam_geometry
\tparam Point \tparam_point
\tparam Strategy \tparam_strategy{Centroid}
\param geometry \param_geometry
\param c \param_point \param_set{centroid}
\param strategy \param_strategy{centroid}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{[include reference/algorithms/centroid_strategies.qbk]}
}

*/
template<typename Geometry, typename Point, typename Strategy>
inline void centroid(Geometry const& geometry, Point& c,
        Strategy const& strategy)
{
    //BOOST_CONCEPT_ASSERT( (geometry::concept::CentroidStrategy<Strategy>) );

    concept::check_concepts_and_equal_dimensions<Point, Geometry const>();

    typedef typename point_type<Geometry>::type point_type;

    // Call dispatch apply method. That one returns true if centroid
    // should be taken from state.
    dispatch::centroid
        <
            typename tag<Geometry>::type,
            Geometry,
            Point,
            Strategy
        >::apply(geometry, c, strategy);
}


/*!
\brief \brief_calc{centroid}
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_default_strategy
\tparam Geometry \tparam_geometry
\tparam Point \tparam_point
\param geometry \param_geometry
\param c The calculated centroid will be assigned to this point reference

\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{
[heading Example]
[centroid]
[centroid_output]
}
 */
template<typename Geometry, typename Point>
inline void centroid(Geometry const& geometry, Point& c)
{
    concept::check_concepts_and_equal_dimensions<Point, Geometry const>();

    typedef typename strategy::centroid::services::default_strategy
        <
            typename cs_tag<Geometry>::type,
            typename tag_cast
                <
                    typename tag<Geometry>::type,
                    pointlike_tag,
                    linear_tag,
                    areal_tag
                >::type,
            dimension<Geometry>::type::value,
            Point,
            Geometry
        >::type strategy_type;

    centroid(geometry, c, strategy_type());
}


/*!
\brief \brief_calc{centroid}
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_return{centroid}.
\tparam Point \tparam_point
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{centroid}

\qbk{[include reference/algorithms/centroid.qbk]}
 */
template<typename Point, typename Geometry>
inline Point return_centroid(Geometry const& geometry)
{
    concept::check_concepts_and_equal_dimensions<Point, Geometry const>();

    Point c;
    centroid(geometry, c);
    return c;
}

/*!
\brief \brief_calc{centroid} \brief_strategy
\ingroup centroid
\details \details_calc{centroid,geometric center (or: center of mass)}. \details_return{centroid}. \details_strategy_reasons
\tparam Point \tparam_point
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{centroid}
\param geometry \param_geometry
\param strategy \param_strategy{centroid}
\return \return_calc{centroid}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/centroid.qbk]}
\qbk{[include reference/algorithms/centroid_strategies.qbk]}
 */
template<typename Point, typename Geometry, typename Strategy>
inline Point return_centroid(Geometry const& geometry, Strategy const& strategy)
{
    //BOOST_CONCEPT_ASSERT( (geometry::concept::CentroidStrategy<Strategy>) );

    concept::check_concepts_and_equal_dimensions<Point, Geometry const>();

    Point c;
    centroid(geometry, c, strategy);
    return c;
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CENTROID_HPP
