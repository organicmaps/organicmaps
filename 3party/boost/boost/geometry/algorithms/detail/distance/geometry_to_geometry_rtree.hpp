// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_GEOMETRY_TO_GEOMETRY_RTREE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_GEOMETRY_TO_GEOMETRY_RTREE_HPP

#include <cstddef>
#include <algorithm>
#include <iterator>

#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>
#include <boost/geometry/iterators/has_one_element.hpp>

#include <boost/geometry/algorithms/for_each.hpp>
#include <boost/geometry/algorithms/intersects.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/index/rtree.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{



template
<
    typename RTreePoint,
    typename Geometry,
    typename Strategy
>
class point_range_to_geometry_rtree
{
private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            RTreePoint,
            typename point_type<Geometry>::type
        >::type comparable_return_type;

    typedef index::rtree<RTreePoint, index::linear<8> > r_tree;

    // functor to evaluate minimum comparable distance
    struct minimum_comparable_distance_evaluator
    {
        r_tree const& m_r_tree;
        comparable_strategy const& m_cstrategy;
        bool m_first;
        comparable_return_type m_min_cd;

        minimum_comparable_distance_evaluator
        (r_tree const& r_tree, comparable_strategy const& cstrategy)
            : m_r_tree(r_tree)
            , m_cstrategy(cstrategy)
            , m_first(true)
            , m_min_cd()
        {}

        template <typename QueryGeometry>
        inline void operator()(QueryGeometry const& query_geometry)
        {
            typename r_tree::value_type t_v;
            std::size_t n =
                m_r_tree.query(index::nearest(query_geometry, 1), &t_v);

            BOOST_ASSERT( n > 0 );

            comparable_return_type cd = dispatch::distance
                <
                    typename r_tree::value_type,
                    QueryGeometry,
                    comparable_strategy
                >::apply(t_v, query_geometry, m_cstrategy);

            if ( m_first || cd < m_min_cd )
            {
                m_first = false;
                m_min_cd = cd;
            }
        }
    };



    // class to choose between for_each_point and for_each_segment
    template <typename G, typename Tag = typename tag<G>::type>
    struct for_each_selector
    {
        typedef dispatch::for_each_segment<G> type;
    };

    template <typename MultiPoint>
    struct for_each_selector<MultiPoint, multi_point_tag>
    {
        typedef dispatch::for_each_point<MultiPoint> type;
    };

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            RTreePoint,
            typename point_type<Geometry>::type
        >::type return_type;

    template <typename PointIterator>
    static inline return_type apply(PointIterator points_first,
                                    PointIterator points_beyond,
                                    Geometry const& geometry,
                                    Strategy const& strategy)
    {
        BOOST_ASSERT( points_first != points_beyond );

        if ( geometry::has_one_element(points_first, points_beyond) )
        {
            return dispatch::distance
                <
                    typename std::iterator_traits<PointIterator>::value_type,
                    Geometry,
                    Strategy
                >::apply(*points_first, geometry, strategy);
        }

        // create -- packing algorithm
        r_tree rt(points_first, points_beyond);

        minimum_comparable_distance_evaluator
            functor(rt,
                    strategy::distance::services::get_comparable
                        <
                            Strategy
                        >::apply(strategy));

        for_each_selector<Geometry>::type::apply(geometry, functor);

        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy, RTreePoint, Geometry
            >::apply(functor.m_min_cd);
    }
};



template
<
    typename Geometry1,
    typename Geometry2,
    typename Strategy
>
class geometry_to_geometry_rtree
{
    // the following works with linear geometries seen as ranges of points
    //
    // we compute the r-tree for the points of one range and then,
    // compute nearest points for the segments of the other,
    // ... and ...
    // vice versa.

private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type comparable_return_type;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type return_type;   

    static inline return_type apply(Geometry1 const& geometry1,
                                    Geometry2 const& geometry2,
                                    Strategy const& strategy,
                                    bool check_intersection = true)
    {
        point_iterator<Geometry1 const> first1 = points_begin(geometry1);
        point_iterator<Geometry1 const> beyond1 = points_end(geometry1);
        point_iterator<Geometry2 const> first2 = points_begin(geometry2);
        point_iterator<Geometry2 const> beyond2 = points_end(geometry2);
        
        if ( geometry::has_one_element(first1, beyond1) )
        {
            return dispatch::distance
                <
                    typename point_type<Geometry1>::type,
                    Geometry2,
                    Strategy
                >::apply(*first1, geometry2, strategy);
        }

        if ( geometry::has_one_element(first2, beyond2) )
        {
            return dispatch::distance
                <
                    typename point_type<Geometry2>::type,
                    Geometry1,
                    Strategy
                >::apply(*first2, geometry1, strategy);
        }

        if ( check_intersection && geometry::intersects(geometry1, geometry2) )
        {
            return return_type(0);
        }

        comparable_strategy cstrategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);

        comparable_return_type cdist1 = point_range_to_geometry_rtree
            <
                typename point_type<Geometry1>::type,
                Geometry2,
                comparable_strategy
            >::apply(first1, beyond1, geometry2, cstrategy);

        comparable_return_type cdist2 = point_range_to_geometry_rtree
            <
                typename point_type<Geometry2>::type,
                Geometry1,
                comparable_strategy
            >::apply(first2, beyond2, geometry1, cstrategy);


        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy, Geometry1, Geometry2
            >::apply( (std::min)(cdist1, cdist2) );
    }
};




}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Linestring1, typename Linestring2, typename Strategy>
struct distance
    <
        Linestring1, Linestring2, Strategy,
        linestring_tag, linestring_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Linestring1, Linestring2, Strategy
        >
{};



template <typename Linestring, typename Polygon, typename Strategy>
struct distance
    <
        Linestring, Polygon, Strategy,
        linestring_tag, polygon_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Linestring, Polygon, Strategy
        >
{};



template <typename Linestring, typename Ring, typename Strategy>
struct distance
    <
        Linestring, Ring, Strategy,
        linestring_tag, ring_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Linestring, Ring, Strategy
        >
{};



template <typename Polygon1, typename Polygon2, typename Strategy>
struct distance
    <
        Polygon1, Polygon2, Strategy,
        polygon_tag, polygon_tag,
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Polygon1, Polygon2, Strategy
        >
{};



template <typename Polygon, typename Ring, typename Strategy>
struct distance
    <
        Polygon, Ring, Strategy,
        polygon_tag, ring_tag,
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Polygon, Ring, Strategy
        >
{};



template <typename Ring1, typename Ring2, typename Strategy>
struct distance
    <
        Ring1, Ring2, Strategy,
        ring_tag, ring_tag,
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::geometry_to_geometry_rtree
        <
            Ring1, Ring2, Strategy
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_GEOMETRY_TO_GEOMETRY_RTREE_HPP
