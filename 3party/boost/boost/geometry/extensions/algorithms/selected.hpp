// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_SELECTED_HPP
#define BOOST_GEOMETRY_ALGORITHMS_SELECTED_HPP

#include <cmath>
#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/strategies/strategies.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/topological_dimension.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>


namespace boost { namespace geometry
{

/*!
    \ingroup impl
 */
#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace selected
{

/*!
\details Checks, per dimension, if d[I] not larger than search distance. If true for all
dimensions then returns true. If larger stops immediately and returns false.
Calculate during this process the sum, which is only valid if returning true
*/
template <typename P1, typename P2, typename T, std::size_t D, std::size_t N>
struct differences_loop
{
    static inline bool apply(P1 const& p1, P2 const& p2, T const& distance, T& sum)
    {
        typedef typename select_coordinate_type<P1, P2>::type coordinate_type;

        coordinate_type const c1 = boost::numeric_cast<coordinate_type>(get<D>(p1));
        coordinate_type const c2 = boost::numeric_cast<coordinate_type>(get<D>(p2));

        T const d = geometry::math::abs(c1 - c2);
        if (d > distance)
        {
            return false;
        }
        sum += d * d;
        return differences_loop<P1, P2, T, D + 1, N>::apply(p1, p2, distance, sum);
    }
};

template <typename P1, typename P2, typename T, std::size_t N>
struct differences_loop<P1, P2, T, N, N>
{
    static inline bool apply(P1 const&, P2 const&, T const&, T&)
    {
        return true;
    }
};



template <typename PS, typename P, typename T, std::size_t D, std::size_t N>
struct outside_loop
{
    static inline bool apply(PS const& seg1, PS const& seg2, P const& point, T const& distance)
    {
        typedef typename select_coordinate_type<PS, P>::type coordinate_type;

        coordinate_type const v = boost::numeric_cast<coordinate_type>(get<D>(point));
        coordinate_type const s1 = get<D>(seg1);
        coordinate_type const s2 = get<D>(seg2);

        // Out of reach if left/bottom or right/top of both points making up the segment
        // I know and currently accept that these comparisons/calculations are done twice per point

        if ((v < s1 - distance && v < s2 - distance) || (v > s1 + distance && v > s2 + distance))
        {
            return true;
        }
        return outside_loop<PS, P, T, D + 1, N>::apply(seg1, seg2, point, distance);
    }
};

template <typename PS, typename P, typename T, std::size_t N>
struct outside_loop<PS, P, T, N, N>
{
    static inline bool apply(PS const&, PS const&, P const&, T const&)
    {
        return false;
    }
};


template <typename P1, typename P2, typename T>
struct close_to_point
{
    static inline bool apply(P1 const& point, P1 const& selection_point, T const& search_radius)
    {
        assert_dimension_equal<P1, P2>();

        T sum = 0;
        if (differences_loop
                <
                    P1, P2, T, 0, dimension<P1>::type::value
                >::apply(point, selection_point, search_radius, sum))
        {
            return sum <= search_radius * search_radius;
        }

        return false;
    }
};

template <typename PS, typename P, typename T>
struct close_to_segment
{
    static inline bool apply(PS const& seg1, PS const& seg2, P const& selection_point, T const& search_radius)
    {
        assert_dimension_equal<PS, P>();

        if (! outside_loop
                <
                    PS, P, T, 0, dimension<P>::type::value
                >::apply(seg1, seg2, selection_point, search_radius))
        {
            // Not outside, calculate dot product/square distance to segment.
            // Call corresponding strategy
            typedef typename strategy::distance::services::default_strategy
                <
                    point_tag, segment_tag, P, PS
                >::type strategy_type;
            typedef typename strategy::distance::services::return_type<strategy_type, P, PS>::type return_type;

            strategy_type strategy;
            return_type result = strategy.apply(selection_point, seg1, seg2);
            return result < search_radius;
        }

        return false;
    }
};

template <typename R, typename P, typename T>
struct close_to_range
{
    static inline bool apply(R const& range, P const& selection_point, T const& search_radius)
    {
        assert_dimension_equal<R, P>();

        std::size_t const n = boost::size(range);
        if (n == 0)
        {
            // Line with zero points, never close
            return false;
        }

        typedef typename point_type<R>::type point_type;
        typedef typename boost::range_iterator<R const>::type iterator_type;

        iterator_type it = boost::begin(range);
        if (n == 1)
        {
            // Line with one point ==> close to point
            return close_to_point<P, point_type, T>::apply(*it, selection_point, search_radius);
        }

        iterator_type previous = it++;
        while(it != boost::end(range))
        {
            //typedef segment<point_type const> segment_type;
            //segment_type s(*previous, *it);
            if (close_to_segment
                    <
                        point_type, P, T
                    >::apply(*previous, *it, selection_point, search_radius))
            {
                return true;
            }
            previous = it++;
        }

        return false;
    }
};

template <typename Tag, typename G, typename P, typename T>
struct use_within
{
    static inline bool apply(G const& geometry, P const& selection_point, T const& search_radius)
    {
        return geometry::within(selection_point, geometry);
    }
};

}} // namespace detail::selected
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

/*!
    \tparam TD topological dimension
 */
template <typename Tag, typename G, std::size_t D, typename P, typename T>
struct selected
{
};

template <typename P1, typename P2, typename T>
struct selected<point_tag, P1, 0, P2, T> : detail::selected::close_to_point<P1, P2, T> { };

// SEGMENT, TODO HERE (close_to_segment)

template <typename L, typename P, typename T>
struct selected<linestring_tag, L, 1, P, T> : detail::selected::close_to_range<L, P, T> { };

template <typename Tag, typename G, typename P, typename T>
struct selected<Tag, G, 2, P, T> : detail::selected::use_within<Tag, G, P, T> { };

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Checks if one geometry is selected by a point lying within or in the neighborhood of that geometry
    \ingroup selected
    \tparam Geometry type of geometry to check
    \tparam Point type of point to check
    \tparam T type of search radius
    \param geometry geometry which might be located in the neighborhood
    \param selection_point point to select the geometry
    \param search_radius for points/linestrings: defines radius of "neighborhood" to find things in
    \return true if point is within or close to the other geometry

 */
template<typename Geometry, typename Point, typename RadiusType>
inline bool selected(Geometry const& geometry,
        Point const& selection_point,
        RadiusType const& search_radius)
{
    concept::check<Geometry const>();
    concept::check<Point const>();

    typedef dispatch::selected
        <
            typename tag<Geometry>::type,
            Geometry,
            topological_dimension<Geometry>::value,
            Point,
            RadiusType
        > selector_type;

    return selector_type::apply(geometry, selection_point, search_radius);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_SELECTED_HPP
