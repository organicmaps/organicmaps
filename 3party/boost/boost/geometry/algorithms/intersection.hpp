// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_INTERSECTION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_INTERSECTION_HPP


#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/algorithms/detail/overlay/intersection_insert.hpp>
#include <boost/geometry/algorithms/intersects.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{

template
<
    typename Box1, typename Box2,
    typename BoxOut,
    typename Strategy,
    std::size_t Dimension, std::size_t DimensionCount
>
struct intersection_box_box
{
    static inline bool apply(Box1 const& box1,
            Box2 const& box2, BoxOut& box_out,
            Strategy const& strategy)
    {
        typedef typename coordinate_type<BoxOut>::type ct;

        ct min1 = get<min_corner, Dimension>(box1);
        ct min2 = get<min_corner, Dimension>(box2);
        ct max1 = get<max_corner, Dimension>(box1);
        ct max2 = get<max_corner, Dimension>(box2);

        if (max1 < min2 || max2 < min1)
        {
            return false;
        }
        // Set dimensions of output coordinate
        set<min_corner, Dimension>(box_out, min1 < min2 ? min2 : min1);
        set<max_corner, Dimension>(box_out, max1 > max2 ? max2 : max1);

        return intersection_box_box
            <
                Box1, Box2, BoxOut, Strategy,
                Dimension + 1, DimensionCount
            >::apply(box1, box2, box_out, strategy);
    }
};

template
<
    typename Box1, typename Box2,
    typename BoxOut,
    typename Strategy,
    std::size_t DimensionCount
>
struct intersection_box_box<Box1, Box2, BoxOut, Strategy, DimensionCount, DimensionCount>
{
    static inline bool apply(Box1 const&, Box2 const&, BoxOut&, Strategy const&)
    {
        return true;
    }
};


}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// By default, all is forwarded to the intersection_insert-dispatcher
template
<
    typename Tag1, typename Tag2, typename TagOut,
    typename Geometry1, typename Geometry2,
    typename GeometryOut,
    typename Strategy
>
struct intersection
{
    typedef std::back_insert_iterator<GeometryOut> output_iterator;

    static inline bool apply(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            GeometryOut& geometry_out,
            Strategy const& strategy)
    {
        typedef typename boost::range_value<GeometryOut>::type OneOut;

        intersection_insert
        <
            Tag1, Tag2, typename geometry::tag<OneOut>::type,
            geometry::is_areal<Geometry1>::value,
            geometry::is_areal<Geometry2>::value,
            geometry::is_areal<OneOut>::value,
            Geometry1, Geometry2,
            detail::overlay::do_reverse<geometry::point_order<Geometry1>::value, false>::value,
            detail::overlay::do_reverse<geometry::point_order<Geometry2>::value, false>::value,
            detail::overlay::do_reverse<geometry::point_order<OneOut>::value>::value,
            output_iterator, OneOut,
            overlay_intersection,
            Strategy
        >::apply(geometry1, geometry2, std::back_inserter(geometry_out), strategy);

        return true;
    }

};


template
<
    typename Box1, typename Box2,
    typename BoxOut,
    typename Strategy
>
struct intersection
    <
        box_tag, box_tag, box_tag,
        Box1, Box2, BoxOut,
        Strategy
    > : public detail::intersection::intersection_box_box
            <
                Box1, Box2, BoxOut,
                Strategy,
                0, geometry::dimension<Box1>::value
            >
{};


template
<
    typename Tag1, typename Tag2, typename TagOut,
    typename Geometry1, typename Geometry2,
    typename GeometryOut,
    typename Strategy
>
struct intersection_reversed
{
    static inline bool apply(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            GeometryOut& geometry_out,
            Strategy const& strategy)
    {
        return intersection
            <
                Tag2, Tag1, TagOut,
                Geometry2, Geometry1,
                GeometryOut, Strategy
            >::apply(geometry2, geometry1, geometry_out, strategy);
    }
};




} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection, spatial set theoretic intersection}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam GeometryOut Collection of geometries (e.g. std::vector, std::deque, boost::geometry::multi*) of which
    the value_type fulfills a \p_l_or_c concept, or it is the output geometry (e.g. for a box)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param geometry_out The output geometry, either a multi_point, multi_polygon,
    multi_linestring, or a box (for intersection of two boxes)

\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename Geometry1,
    typename Geometry2,
    typename GeometryOut
>
inline bool intersection(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            GeometryOut& geometry_out)
{
    concept::check<Geometry1 const>();
    concept::check<Geometry2 const>();

    typedef strategy_intersection
        <
            typename cs_tag<Geometry1>::type,
            Geometry1,
            Geometry2,
            typename geometry::point_type<Geometry1>::type
        > strategy;


    return boost::mpl::if_c
        <
            geometry::reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::intersection_reversed
            <
                    typename geometry::tag<Geometry1>::type,
                    typename geometry::tag<Geometry2>::type,
                    typename geometry::tag<GeometryOut>::type,
                    Geometry1, Geometry2, GeometryOut, strategy
            >,
            dispatch::intersection
            <
                    typename geometry::tag<Geometry1>::type,
                    typename geometry::tag<Geometry2>::type,
                    typename geometry::tag<GeometryOut>::type,
                    Geometry1, Geometry2, GeometryOut, strategy
            >
        >::type::apply(geometry1, geometry2, geometry_out, strategy());
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_INTERSECTION_HPP
