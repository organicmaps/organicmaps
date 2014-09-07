// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ITERATORS_POINT_ITERATOR_TYPE_HPP
#define BOOST_GEOMETRY_ITERATORS_POINT_ITERATOR_TYPE_HPP

#include <boost/geometry/iterators/dispatch/point_iterator_type.hpp>
#include <boost/geometry/iterators/dispatch/point_iterator.hpp>

#include <boost/range.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/if.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/iterators/flatten_iterator.hpp>
#include <boost/geometry/iterators/concatenate_iterator.hpp>


namespace boost { namespace geometry
{



#ifndef DOXYGEN_NO_DETAIL
namespace detail_dispatch
{


template <typename Geometry>
struct point_iterator_value_type
{
    typedef typename boost::mpl::if_c
        <
            !boost::is_const<Geometry>::type::value,
            typename geometry::point_type<Geometry>::type,
            typename geometry::point_type<Geometry>::type const
        >::type type;
};




template
<
    typename Geometry, 
    typename Tag = typename tag<Geometry>::type
>
struct point_iterator_inner_range_type
{
    typedef typename boost::mpl::if_c
        <
            !boost::is_const<Geometry>::type::value,
            typename boost::range_value<Geometry>::type,
            typename boost::range_value<Geometry>::type const
        >::type type;
};


template <typename Polygon>
struct point_iterator_inner_range_type<Polygon, polygon_tag>
{
    typedef typename boost::mpl::if_c
        <
            !boost::is_const<Polygon>::type::value,
            typename geometry::ring_type<Polygon>::type,
            typename geometry::ring_type<Polygon>::type const
        >::type type;
};



} // namespace detail_dispatch
#endif // DOXYGEN_NO_DETAIL





#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Linestring>
struct point_iterator_type<Linestring, linestring_tag>
{
    typedef typename boost::range_iterator<Linestring>::type type;
};


template <typename Ring>
struct point_iterator_type<Ring, ring_tag>
{
    typedef typename boost::range_iterator<Ring>::type type;
};


template <typename Polygon>
class point_iterator_type<Polygon, polygon_tag>
{
private:
    typedef typename detail_dispatch::point_iterator_inner_range_type
        <
            Polygon
        >::type inner_range;

public:
    typedef concatenate_iterator
        <
            typename boost::range_iterator<inner_range>::type,
            flatten_iterator
                <
                    typename boost::range_iterator
                        <
                            typename geometry::interior_type<Polygon>::type
                        >::type,
                    typename dispatch::point_iterator_type
                        <
                            inner_range
                        >::type,
                    typename detail_dispatch::point_iterator_value_type
                        <
                            Polygon
                        >::type,
                    dispatch::points_begin<inner_range>,
                    dispatch::points_end<inner_range>
                >,
            typename detail_dispatch::point_iterator_value_type<Polygon>::type
        > type;
};


template <typename MultiPoint>
struct point_iterator_type<MultiPoint, multi_point_tag>
{
    typedef typename boost::range_iterator<MultiPoint>::type type;
};


template <typename MultiLinestring>
class point_iterator_type<MultiLinestring, multi_linestring_tag>
{
private:
    typedef typename detail_dispatch::point_iterator_inner_range_type
        <
            MultiLinestring
        >::type inner_range;

public:
    typedef flatten_iterator
        <
            typename boost::range_iterator<MultiLinestring>::type,
            typename dispatch::point_iterator_type<inner_range>::type,
            typename detail_dispatch::point_iterator_value_type
                <
                    MultiLinestring
                >::type,
            dispatch::points_begin<inner_range>,
            dispatch::points_end<inner_range>
        > type;
};


template <typename MultiPolygon>
class point_iterator_type<MultiPolygon, multi_polygon_tag>
{
private:
    typedef typename detail_dispatch::point_iterator_inner_range_type
        <
            MultiPolygon
        >::type inner_range;

public:
    typedef flatten_iterator
        <
            typename boost::range_iterator<MultiPolygon>::type,
            typename dispatch::point_iterator_type<inner_range>::type,
            typename detail_dispatch::point_iterator_value_type
                <
                    MultiPolygon
                >::type,
            dispatch::points_begin<inner_range>,
            dispatch::points_end<inner_range>
        > type;
};





} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ITERATORS_POINT_ITERATOR_TYPE_HPP
