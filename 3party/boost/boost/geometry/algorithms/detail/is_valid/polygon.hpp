// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POLYGON_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POLYGON_HPP

#include <cstddef>

#include <algorithm>
#include <deque>
#include <iterator>
#include <set>

#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>

#include <boost/geometry/algorithms/detail/is_valid/complement_graph.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_valid_self_turns.hpp>
#include <boost/geometry/algorithms/detail/is_valid/is_acceptable_turn.hpp>
#include <boost/geometry/algorithms/detail/is_valid/ring.hpp>

#include <boost/geometry/algorithms/detail/is_valid/debug_print_turns.hpp>
#include <boost/geometry/algorithms/detail/is_valid/debug_validity_phase.hpp>
#include <boost/geometry/algorithms/detail/is_valid/debug_complement_graph.hpp>

#include <boost/geometry/algorithms/dispatch/is_valid.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template
<
    typename Polygon,
    bool AllowDuplicates,
    bool CheckRingValidityOnly = false
>
class is_valid_polygon
{
protected:
    typedef debug_validity_phase<Polygon> debug_phase;



    template <typename InteriorRings>
    static bool has_valid_interior_rings(InteriorRings const& interior_rings)
    {
        return
            detail::check_iterator_range
                <
                    detail::is_valid::is_valid_ring
                        <
                            typename boost::range_value<InteriorRings>::type,
                            AllowDuplicates,
                            false, // do not check self-intersections
                            true // indicate that the ring is interior
                        >
                >::apply(boost::begin(interior_rings),
                         boost::end(interior_rings));
    }

    struct has_valid_rings
    {
        static inline bool apply(Polygon const& polygon)
        {
            typedef typename ring_type<Polygon>::type ring_type;

            // check validity of exterior ring
            debug_phase::apply(1);

            if ( !detail::is_valid::is_valid_ring
                     <
                         ring_type,
                         AllowDuplicates,
                         false // do not check self intersections
                     >::apply(exterior_ring(polygon)) )
            {
                return false;
            }

            // check validity of interior rings
            debug_phase::apply(2);

            return has_valid_interior_rings(geometry::interior_rings(polygon));
        }
    };




    template
    <
        typename RingIterator,
        typename ExteriorRing,
        typename TurnIterator
    >
    static inline bool are_holes_inside(RingIterator rings_first,
                                        RingIterator rings_beyond,
                                        ExteriorRing const& exterior_ring,
                                        TurnIterator turns_first,
                                        TurnIterator turns_beyond)
    {
        // collect the interior ring indices that have turns with the
        // exterior ring
        std::set<int> ring_indices;
        for (TurnIterator tit = turns_first; tit != turns_beyond; ++tit)
        {
            if ( tit->operations[0].seg_id.ring_index == -1 )
            {
                BOOST_ASSERT( tit->operations[0].other_id.ring_index != -1 );
                ring_indices.insert(tit->operations[0].other_id.ring_index);
            }
            else if ( tit->operations[0].other_id.ring_index == -1 )
            {
                BOOST_ASSERT( tit->operations[0].seg_id.ring_index != -1 );
                ring_indices.insert(tit->operations[0].seg_id.ring_index);
            }
        }

        int ring_index = 0;
        for (RingIterator it = rings_first; it != rings_beyond;
             ++it, ++ring_index)
        {
            // do not examine interior rings that have turns with the
            // exterior ring
            if (  ring_indices.find(ring_index) == ring_indices.end()
                  && !geometry::covered_by(range::front(*it), exterior_ring) )
            {
                return false;
            }
        }

        // collect all rings (exterior and/or interior) that have turns
        for (TurnIterator tit = turns_first; tit != turns_beyond; ++tit)
        {
            ring_indices.insert(tit->operations[0].seg_id.ring_index);
            ring_indices.insert(tit->operations[0].other_id.ring_index);
        }

        ring_index = 0;
        for (RingIterator it1 = rings_first; it1 != rings_beyond;
             ++it1, ++ring_index)
        {
            // do not examine rings that are associated with turns
            if ( ring_indices.find(ring_index) == ring_indices.end() )
            {
                for (RingIterator it2 = rings_first; it2 != rings_beyond; ++it2)
                {
                    if ( it1 != it2
                         && geometry::within(range::front(*it1), *it2) )
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    template
    <
        typename InteriorRings,
        typename ExteriorRing,
        typename TurnIterator
    >
    static inline bool are_holes_inside(InteriorRings const& interior_rings,
                                        ExteriorRing const& exterior_ring,
                                        TurnIterator first,
                                        TurnIterator beyond)
    {
        return are_holes_inside(boost::begin(interior_rings),
                                boost::end(interior_rings),
                                exterior_ring,
                                first,
                                beyond);
    }

    struct has_holes_inside
    {    
        template <typename TurnIterator>
        static inline bool apply(Polygon const& polygon,
                                 TurnIterator first,
                                 TurnIterator beyond)
        {
            return are_holes_inside(geometry::interior_rings(polygon),
                                    geometry::exterior_ring(polygon),
                                    first,
                                    beyond);
        }
    };




    struct has_connected_interior
    {
        template <typename TurnIterator>
        static inline bool apply(Polygon const& polygon,
                                 TurnIterator first,
                                 TurnIterator beyond)
        {
            typedef typename std::iterator_traits
                <
                    TurnIterator
                >::value_type turn_type;
            typedef complement_graph<typename turn_type::point_type> graph;

            graph g(geometry::num_interior_rings(polygon) + 1);
            for (TurnIterator tit = first; tit != beyond; ++tit)
            {
                typename graph::vertex_handle v1 = g.add_vertex
                    ( tit->operations[0].seg_id.ring_index + 1 );
                typename graph::vertex_handle v2 = g.add_vertex
                    ( tit->operations[0].other_id.ring_index + 1 );
                typename graph::vertex_handle vip = g.add_vertex(tit->point);

                g.add_edge(v1, vip);
                g.add_edge(v2, vip);
            }

            debug_print_complement_graph(std::cout, g);

            return !g.has_cycles();
        }
    };

public:
    static inline bool apply(Polygon const& polygon)
    {
        if ( !has_valid_rings::apply(polygon) )
        {
            return false;
        }

        if ( CheckRingValidityOnly )
        {
            return true;
        }

        // compute turns and check if all are acceptable
        debug_phase::apply(3);

        typedef has_valid_self_turns<Polygon> has_valid_turns;

        std::deque<typename has_valid_turns::turn_type> turns;
        bool has_invalid_turns = !has_valid_turns::apply(polygon, turns);
        debug_print_turns(turns.begin(), turns.end());

        if ( has_invalid_turns )
        {
            return false;
        }

        // check if all interior rings are inside the exterior ring
        debug_phase::apply(4);

        if ( !has_holes_inside::apply(polygon, turns.begin(), turns.end()) )
        {
            return false;
        }

        // check whether the interior of the polygon is a connected set
        debug_phase::apply(5);

        return has_connected_interior::apply(polygon,
                                             turns.begin(),
                                             turns.end());
    }
};


}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A Polygon is always a simple geometric object provided that it is valid.
//
// Reference (for validity of Polygons): OGC 06-103r4 (6.1.11.1)
template <typename Polygon, bool AllowSpikes, bool AllowDuplicates>
struct is_valid<Polygon, polygon_tag, AllowSpikes, AllowDuplicates>
    : detail::is_valid::is_valid_polygon<Polygon, AllowDuplicates>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_POLYGON_HPP
