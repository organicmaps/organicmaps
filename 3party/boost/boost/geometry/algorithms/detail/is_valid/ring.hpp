// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_RING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_RING_HPP

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_order.hpp>

#include <boost/geometry/util/order_as_direction.hpp>
#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/equals.hpp>

#include <boost/geometry/views/reversible_view.hpp>
#include <boost/geometry/views/closeable_view.hpp>

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_spikes.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_duplicates.hpp>

#include <boost/geometry/strategies/area.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


// struct to check whether a ring is topologically closed
template <typename Ring, closure_selector Closure /* open */>
struct is_topologically_closed
{
    static inline bool apply(Ring const&)
    {
        return true;
    }
};

template <typename Ring>
struct is_topologically_closed<Ring, closed>
{
    static inline bool apply(Ring const& ring)
    {
        return geometry::equals(range::front(ring), range::back(ring));
    }
};



template <typename ResultType, bool IsInteriorRing /* false */>
struct ring_area_predicate
{
    typedef std::greater<ResultType> type;
};

template <typename ResultType>
struct ring_area_predicate<ResultType, true>
{
    typedef std::less<ResultType> type;
};



template <typename Ring, bool IsInteriorRing>
struct is_properly_oriented
{
    typedef typename point_type<Ring>::type point_type;

    typedef typename strategy::area::services::default_strategy
        <
            typename cs_tag<point_type>::type,
            point_type
        >::type strategy_type;

    typedef detail::area::ring_area
        <
            order_as_direction<geometry::point_order<Ring>::value>::value,
            geometry::closure<Ring>::value
        > ring_area_type;

    typedef typename default_area_result<Ring>::type area_result_type;

    static inline bool apply(Ring const& ring)
    {
        typename ring_area_predicate
            <
                area_result_type, IsInteriorRing
            >::type predicate;

        // Check area
        area_result_type const zero = area_result_type();
        return predicate(ring_area_type::apply(ring, strategy_type()), zero);
    }
};



template
<
    typename Ring,
    bool AllowDuplicates,
    bool CheckSelfIntersections = true,
    bool IsInteriorRing = false
>
struct is_valid_ring
{
    static inline bool apply(Ring const& ring)
    {
        // return invalid if any of the following condition holds:
        // (a) the ring's size is below the minimal one
        // (b) the ring is not topologically closed
        // (c) the ring has spikes
        // (d) the ring has duplicate points (if AllowDuplicates is false)
        // (e) the boundary of the ring has self-intersections
        // (f) the order of the points is inconsistent with the defined order
        //
        // Note: no need to check if the area is zero. If this is the
        // case, then the ring must have at least two spikes, which is
        // checked by condition (c).

        closure_selector const closure = geometry::closure<Ring>::value;

        return
            ( boost::size(ring)
              >= core_detail::closure::minimum_ring_size<closure>::value )
            && is_topologically_closed<Ring, closure>::apply(ring) 
            && (AllowDuplicates || !has_duplicates<Ring, closure>::apply(ring))
            && !has_spikes<Ring, closure>::apply(ring)
            && !(CheckSelfIntersections && geometry::intersects(ring))
            && is_properly_oriented<Ring, IsInteriorRing>::apply(ring);
    }
};


}} // namespace dispatch
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// A Ring is a Polygon with exterior boundary only.
// The Ring's boundary must be a LinearRing (see OGC 06-103-r4,
// 6.1.7.1, for the definition of LinearRing)
//
// Reference (for polygon validity): OGC 06-103r4 (6.1.11.1)
template <typename Ring, bool AllowSpikes, bool AllowDuplicates>
struct is_valid<Ring, ring_tag, AllowSpikes, AllowDuplicates>
    : detail::is_valid::is_valid_ring<Ring, AllowDuplicates>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_RING_HPP
