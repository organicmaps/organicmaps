// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2014.
// Modifications copyright (c) 2013-2014, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_AREAL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_AREAL_HPP

#include <iterator>

#include <boost/range.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>

#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/segment_box.hpp>
#include <boost/geometry/algorithms/detail/disjoint/linear_segment_or_box.hpp>

#include <boost/geometry/algorithms/dispatch/disjoint.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


template<typename Geometry1, typename Geometry2>
struct disjoint_linear_areal
{
    static inline bool apply(Geometry1 const& g1, Geometry2 const& g2)
    {
        // if there are intersections - return false
        if ( !disjoint_linear<Geometry1, Geometry2>::apply(g1, g2) )
            return false;

        typedef typename point_type<Geometry1>::type point1_type;
        point1_type p;
        geometry::point_on_border(p, g1);
        return !geometry::covered_by(p, g2);
    }
};




template
<
    typename Segment,
    typename Areal,
    typename Tag = typename tag<Areal>::type
>
struct disjoint_segment_areal
    : not_implemented<Segment, Areal>
{};


template <typename Segment, typename Polygon>
class disjoint_segment_areal<Segment, Polygon, polygon_tag>
{
private:
    template <typename RingIterator>
    static inline bool check_interior_rings(RingIterator first,
                                            RingIterator beyond,
                                            Segment const& segment)
    {
        for (RingIterator it = first; it != beyond; ++it)
        {
            if ( !disjoint_range_segment_or_box
                     <
                         typename std::iterator_traits
                             <
                                 RingIterator
                             >::value_type,
                         closure<Polygon>::value,
                         Segment
                     >::apply(*it, segment) )
            {
                return false;
            }
        }
        return true;
    }


    template <typename InteriorRings>
    static inline
    bool check_interior_rings(InteriorRings const& interior_rings,
                              Segment const& segment)
    {
        return check_interior_rings(boost::begin(interior_rings),
                                    boost::end(interior_rings),
                                    segment);
    }


public:
    static inline bool apply(Segment const& segment, Polygon const& polygon)
    {
        typedef typename geometry::ring_type<Polygon>::type ring;

        if ( !disjoint_range_segment_or_box
                 <
                     ring, closure<Polygon>::value, Segment
                 >::apply(geometry::exterior_ring(polygon), segment) )
        {
            return false;
        }

        if ( !check_interior_rings(geometry::interior_rings(polygon), segment) )
        {
            return false;
        }

        typename point_type<Segment>::type p;
        detail::assign_point_from_index<0>(segment, p);

        return !geometry::covered_by(p, polygon);
    }
};


template <typename Segment, typename MultiPolygon>
struct disjoint_segment_areal<Segment, MultiPolygon, multi_polygon_tag>
{
    static inline
    bool apply(Segment const& segment, MultiPolygon const& multipolygon)
    {
        return disjoint_multirange_segment_or_box
            <
                MultiPolygon, Segment
            >::apply(multipolygon, segment);
    }
};


template <typename Segment, typename Ring>
struct disjoint_segment_areal<Segment, Ring, ring_tag>
{
    static inline bool apply(Segment const& segment, Ring const& ring)
    {
        if ( !disjoint_range_segment_or_box
                 <
                     Ring, closure<Ring>::value, Segment
                 >::apply(ring, segment) )
        {
            return false;
        }

        typename point_type<Segment>::type p;
        detail::assign_point_from_index<0>(segment, p);
        
        return !geometry::covered_by(p, ring);        
    }
};


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Linear, typename Areal>
struct disjoint<Linear, Areal, 2, linear_tag, areal_tag, false>
    : public detail::disjoint::disjoint_linear_areal<Linear, Areal>
{};


template <typename Areal, typename Linear>
struct disjoint<Areal, Linear, 2, areal_tag, linear_tag, false>
{    
    static inline
    bool apply(Areal const& areal, Linear const& linear)
    {
        return detail::disjoint::disjoint_linear_areal
            <
                Linear, Areal
            >::apply(linear, areal);
    }
};


template <typename Areal, typename Segment>
struct disjoint<Areal, Segment, 2, areal_tag, segment_tag, false>
{
    static inline bool apply(Areal const& g1, Segment const& g2)
    {
        return detail::disjoint::disjoint_segment_areal
            <
                Segment, Areal
            >::apply(g2, g1);
    }
};


template <typename Segment, typename Areal>
struct disjoint<Segment, Areal, 2, segment_tag, areal_tag, false>
    : detail::disjoint::disjoint_segment_areal<Segment, Areal>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_AREAL_HPP
