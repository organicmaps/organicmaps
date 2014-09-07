// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_MULTIPOLYGON_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_MULTIPOLYGON_HPP

#include <deque>

#include <boost/iterator/filter_iterator.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>

#include <boost/geometry/algorithms/detail/is_valid/has_valid_self_turns.hpp>
#include <boost/geometry/algorithms/detail/is_valid/is_acceptable_turn.hpp>
#include <boost/geometry/algorithms/detail/is_valid/polygon.hpp>

#include <boost/geometry/algorithms/detail/is_valid/debug_print_turns.hpp>
#include <boost/geometry/algorithms/detail/is_valid/debug_validity_phase.hpp>

#include <boost/geometry/algorithms/dispatch/is_valid.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template <typename MultiPolygon, bool AllowDuplicates>
class is_valid_multipolygon
    : is_valid_polygon
        <
            typename boost::range_value<MultiPolygon>::type,
            AllowDuplicates,
            true // check only the validity of rings
        >
{
private:
    typedef is_valid_polygon
        <
            typename boost::range_value<MultiPolygon>::type,
            AllowDuplicates,
            true
        > base;



    template <typename PolygonIterator, typename TurnIterator>
    static inline
    bool are_polygon_interiors_disjoint(PolygonIterator polygons_first,
                                        PolygonIterator polygons_beyond,
                                        TurnIterator turns_first,
                                        TurnIterator turns_beyond)
    {
        std::set<int> multi_indices;
        for (TurnIterator tit = turns_first; tit != turns_beyond; ++tit)
        {
            multi_indices.insert(tit->operations[0].seg_id.multi_index);
            multi_indices.insert(tit->operations[0].other_id.multi_index);
        }

        int multi_index = 0;
        for (PolygonIterator it1 = polygons_first; it1 != polygons_beyond;
             ++it1, ++multi_index)
        {
            if ( multi_indices.find(multi_index) != multi_indices.end() )
            {
                continue;
            }

            for (PolygonIterator it2 = polygons_first;
                 it2 != polygons_beyond; ++it2)
            {
                if ( it1 != it2
                     &&
                     geometry::within(range::front(exterior_ring(*it1)), *it2)
                     )
                {
                    return false;
                }
            }
        }
        return true;
    }



    class has_multi_index
    {
    public:
        has_multi_index(int multi_index)
            : m_multi_index(multi_index)
        {}

        template <typename Turn>
        inline bool operator()(Turn const& turn) const
        {
            return turn.operations[0].seg_id.multi_index == m_multi_index
                && turn.operations[0].other_id.multi_index == m_multi_index;
        }

    private:
        int const m_multi_index;
    };



    template <typename Predicate>
    struct has_property_per_polygon
    {
        template <typename PolygonIterator, typename TurnIterator>
        static inline bool apply(PolygonIterator polygons_first,
                                 PolygonIterator polygons_beyond,
                                 TurnIterator turns_first,
                                 TurnIterator turns_beyond)
        {
            int multi_index = 0;
            for (PolygonIterator it = polygons_first; it != polygons_beyond;
                 ++it, ++multi_index)
            {
                has_multi_index index_predicate(multi_index);

                typedef boost::filter_iterator
                    <
                        has_multi_index, TurnIterator
                    > filtered_turn_iterator;

                filtered_turn_iterator filtered_turns_first(index_predicate,
                                                            turns_first,
                                                            turns_beyond);

                filtered_turn_iterator filtered_turns_beyond(index_predicate,
                                                             turns_beyond,
                                                             turns_beyond);

                if ( !Predicate::apply(*it,
                                       filtered_turns_first,
                                       filtered_turns_beyond) )
                {
                    return false;
                }
            }
            return true;
        }
    };



    template <typename PolygonIterator, typename TurnIterator>
    static inline bool have_holes_inside(PolygonIterator polygons_first,
                                         PolygonIterator polygons_beyond,
                                         TurnIterator turns_first,
                                         TurnIterator turns_beyond)
    {
        return has_property_per_polygon
            <
                typename base::has_holes_inside
            >::apply(polygons_first, polygons_beyond,
                     turns_first, turns_beyond);
    }



    template <typename PolygonIterator, typename TurnIterator>
    static inline bool have_connected_interior(PolygonIterator polygons_first,
                                               PolygonIterator polygons_beyond,
                                               TurnIterator turns_first,
                                               TurnIterator turns_beyond)
    {
        return has_property_per_polygon
            <
                typename base::has_connected_interior
            >::apply(polygons_first, polygons_beyond,
                     turns_first, turns_beyond);
    }


public:
    static inline bool apply(MultiPolygon const& multipolygon)
    {
        typedef debug_validity_phase<MultiPolygon> debug_phase;

        // check validity of all polygons ring
        debug_phase::apply(1);

        if ( !detail::check_iterator_range
                  <
                      base,
                      false // do not allow empty multi-polygons
                  >::apply(boost::begin(multipolygon),
                           boost::end(multipolygon)) )
        {
            return false;
        }


        // compute turns and check if all are acceptable
        debug_phase::apply(2);

        typedef has_valid_self_turns<MultiPolygon> has_valid_turns;

        std::deque<typename has_valid_turns::turn_type> turns;
        bool has_invalid_turns = !has_valid_turns::apply(multipolygon, turns);
        debug_print_turns(turns.begin(), turns.end());

        if ( has_invalid_turns )
        {
            return false;
        }


        // check if each polygon's interior rings are inside the
        // exterior and not one inside the other
        debug_phase::apply(3);

        if ( !have_holes_inside(boost::begin(multipolygon),
                                boost::end(multipolygon),
                                turns.begin(),
                                turns.end()) )
        {
            return false;
        }


        // check that each polygon's interior is connected
        debug_phase::apply(4);

        if ( !have_connected_interior(boost::begin(multipolygon),
                                      boost::end(multipolygon),
                                      turns.begin(),
                                      turns.end()) )
        {
            return false;
        }


        // check if polygon interiors are disjoint
        debug_phase::apply(5);
        return are_polygon_interiors_disjoint(boost::begin(multipolygon),
                                              boost::end(multipolygon),
                                              turns.begin(),
                                              turns.end());
    }
};

}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// Not clear what the definition is.
// Right now we check that each element is simple (in fact valid), and
// that the MultiPolygon is also valid.
//
// Reference (for validity of MultiPolygons): OGC 06-103r4 (6.1.14)
template <typename MultiPolygon, bool AllowSpikes, bool AllowDuplicates>
struct is_valid<MultiPolygon, multi_polygon_tag, AllowSpikes, AllowDuplicates>
    : detail::is_valid::is_valid_multipolygon<MultiPolygon, AllowDuplicates>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_MULTIPOLYGON_HPP
