// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_SPLIT_RINGS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_SPLIT_RINGS_HPP

#define BOOST_GEOMETRY_CHECK_SPLIT_RINGS

#include <deque>
#include <string>

#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_SPLIT_RINGS) || defined(BOOST_GEOMETRY_CHECK_SPLIT_RINGS)
#  include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace split_rings
{

template <typename Range>
struct split_range
{
/*

   1             2
   +-------------+
   |      4     /
   |       |\  /
   |       | \/____ IP
   |       | /\
   |       |/  \
   |      3     \
   +-------------+
  0,6            5

  - we want to split the range at the IP into two rings
  - At the IP: we have segment_indices 2,4 (result of get_turns_in_sections)
  - We want to copy and remove vertices 3,4
  --> count=4-2
  --> copy [3,5) -> copy(begin()+id1+1, begin()+id1+count+1)
  --> erase: idem
  --> insert(begin()+id1+1)

  --> we use id1+1

  After that, we need to update all indices AFTER IP.
  We removed two vertices here (4-2), and added one (the IP)

*/
    static inline void apply(Range& range, Range& output
        , segment_identifier const& id1
        , segment_identifier const& id2
        , typename geometry::point_type<Range>::type const& point
        )
    {
        if (id1.ring_index == id2.ring_index
            && id1.multi_index == id2.multi_index)
        {
            int mn = (std::min)(id1.segment_index, id2.segment_index);
            mn++;

            typename boost::range_iterator<Range>::type first = range.begin();
            first += mn;

            typename boost::range_iterator<Range>::type last = first;
            last += geometry::math::abs(id2.segment_index - id1.segment_index);

            // Create splitted ring
            output.push_back(point);
            std::copy(first, last, std::back_inserter(output));
            output.push_back(point);

            // Remove the loop from the range
            range.erase(first, last);

            // Iterator is invalid because of erasure, construct again
            range.insert(range.begin() + mn, point);
        }
    }
};


/*template <typename Polygon>
struct split_polygon
{
    typedef typename geometry::ring_type<Polygon>::type ring_type;

    static inline void apply(Polygon& polygon, ring_type& splitted
        , segment_identifier const& id1
        , segment_identifier const& id2
        , typename geometry::point_type<Polygon>::type const& point
        )
    {
        if (id1.ring_index == id2.ring_index
            && id1.multi_index == id2.multi_index)
        {
            ring_type& ring = id1.ring_index < 0
                ? geometry::exterior_ring(polygon)
                : geometry::interior_rings(polygon)[id1.ring_index];

            split_range<ring_type>::apply(ring, splitted, id1, id2, point);
        }
    }
};*/


template <typename Tag, typename Geometry>
struct split
{};


template <typename Ring>
struct split<ring_tag, Ring> : split_range<Ring>
{};


//template <typename Polygon>
//struct split<polygon_tag, Polygon> : split_polygon<Polygon>
//{};






template <typename Tag, typename RingCollection, typename Geometry>
struct insert_rings
{};


template <typename RingCollection, typename Ring>
struct insert_rings<ring_tag, RingCollection, Ring>
{
    static inline void apply(RingCollection& ring_collection, Ring const& ring)
    {
#ifdef BOOST_GEOMETRY_DEBUG_SPLIT_RINGS
std::cout << geometry::wkt(ring)
        << " ; " << geometry::area(ring)
        << " " << ring.size()
        //<< " at " << geometry::wkt(first.point)
        << std::endl;
/*std::cout << "geometry "
    << " " << geometry::area(geometry)
    << std::endl;*/
#endif

        ring_collection.push_back(ring);
    }
};


template <typename RingCollection, typename Polygon>
struct insert_rings<polygon_tag, RingCollection, Polygon>
{
    static inline void apply(RingCollection& ring_collection, Polygon const& polygon)
    {
        ring_collection.push_back(exterior_ring(polygon));

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);

        typedef typename boost::range_const_iterator
            <
                typename interior_type<Polygon const>::type
            >::type ring_iterator;

        for (ring_iterator it = boost::begin(rings); it != boost::end(rings); ++it)
        {
#ifdef BOOST_GEOMETRY_DEBUG_SPLIT_RINGS
std::cout << geometry::wkt(*it)
        << " ; " << geometry::area(*it)
        << " " << it->size()
        //<< " at " << geometry::wkt(first.point)
        << std::endl;
/*std::cout << "geometry "
    << " " << geometry::area(geometry)
    << std::endl;*/
#endif

            ring_collection.push_back(*it);
        }
    }
};


/// Sorts vector of turns (results from get_turns)
template <typename Turn>
struct sorter
{
    inline bool operator()(Turn const& left, Turn const& right) const
    {
        if (left.count_between != right.count_between)
        {
            return left.count_between < right.count_between;
        }

        if (left.operations[0].seg_id.segment_index
                == right.operations[0].seg_id.segment_index)
        {
            return left.operations[0].distance < right.operations[0].distance;
        }
        return left.operations[0].seg_id.segment_index
                    < right.operations[0].seg_id.segment_index;
    }
};

/// Turn operation with additional distance field
template <typename P>
struct split_turn_operation : public detail::overlay::turn_operation
{
   inline split_turn_operation()
        : detail::overlay::turn_operation()
        , distance(geometry::return_distance_result<distance_type>(0))
    {}

    typedef typename default_distance_result<P, P>::type distance_type;
    distance_type distance; // distance-measurement from segment.first to IP
};


/// Turn information with distance fields, plus "count_between"  field
template <typename P>
struct split_turn_info : detail::overlay::turn_info
            <
                P, split_turn_operation<P>
            >
{
    //std::string history;
    int count_between; // counts number of segments between ring in intersection

    split_turn_info()
        : count_between(0)
    {}
};


/// Policy to calculate distance
struct split_calculate_distance_policy
{
    template <typename Point1, typename Point2, typename Info>
    static inline void apply(Info& info, Point1 const& p1, Point2 const& p2)
    {
        info.operations[0].distance
                    = geometry::distance(info.point, p1);
        info.operations[1].distance
                    = geometry::distance(info.point, p2);
    }

};


template <typename Range, typename RingCollection>
class range_split_rings
{
    typedef typename geometry::tag<Range>::type tag;
    typedef typename geometry::point_type<Range>::type point_type;

    typedef typename geometry::ring_type<Range>::type ring_type;


    typedef typename strategy_intersection
        <
            typename cs_tag<point_type>::type,
            point_type,
            point_type,
            point_type
        >::segment_intersection_strategy_type strategy;




#ifdef BOOST_GEOMETRY_DEBUG_SPLIT_RINGS
    template <typename Turns>
    static void report(Turns const& turns, std::string const& header)
    {
        if (turns.empty())
        {
            return;
        }
        std::cout << header << std::endl;
        BOOST_FOREACH(typename boost::range_value<Turns>::type const& turn, turns)
        {
            std::cout
                << "I at " << turn.operations[0].seg_id.segment_index
                << "/" << turn.operations[1].seg_id.segment_index
                << " (" << turn.count_between
                << ") " << turn.operations[0].distance
                << "/" << turn.operations[1].distance
                << " " << geometry::wkt(turn.point) << std::endl;
        }
    }
#endif

    template <typename Operation>
    static bool adapt(Operation& op, Operation const& first, Operation const& second)
    {
        if (first.seg_id.segment_index > second.seg_id.segment_index)
        {
            return adapt(op, second, first);
        }
        if (op.seg_id.segment_index > first.seg_id.segment_index
            || (op.seg_id.segment_index == first.seg_id.segment_index
                && op.distance > first.distance)
            )
        {
            if (op.seg_id.segment_index < second.seg_id.segment_index
                || (op.seg_id.segment_index == second.seg_id.segment_index
                    && op.distance < second.distance)
                )
            {
                // mark for deletion
                op.seg_id.segment_index = -1;
                return true;
            }
            else
            {
                op.seg_id.segment_index -= (second.seg_id.segment_index - first.seg_id.segment_index - 1);
            }
        }
        return false;
    }


    static void call(Range range, RingCollection& ring_collection)
    {
        typedef split_turn_info<point_type> turn_info;

        typedef std::deque<turn_info> turns_type;
        turns_type turns;

        detail::get_turns::no_interrupt_policy policy;
        geometry::get_turns
            <
                split_calculate_distance_policy
            >(range, turns, policy);

        //report(turns, "intersected");

        // Make operations[0].seg_id always the smallest, to sort properly
        // Also calculate the number of segments in between
        for (typename boost::range_iterator<turns_type>::type
            it = boost::begin(turns);
            it != boost::end(turns);
            ++it)
        {
            turn_info& turn = *it;
            if (turn.operations[0].seg_id.segment_index > turn.operations[1].seg_id.segment_index)
            {
                std::swap(turn.operations[0], turn.operations[1]);
            }
            // ...[1] > ...[0]
            // check count
            int const between1 = turn.operations[1].seg_id.segment_index
                - turn.operations[0].seg_id.segment_index;
            /*
            NOTE: if we would use between2 here, we have to adapt other code as well,
                    such as adaption of the indexes; splitting of the range, etc.
            int between2 = boost::size(range) + turn.operations[0].seg_id.segment_index
                    - turn.operations[1].seg_id.segment_index;
            turn.count_between = (std::min)(between1, between2);
            */

            turn.count_between = between1;
        }
        //report(turns, "swapped");

        std::sort(turns.begin(), turns.end(), sorter<turn_info>());
        //report(turns, "sorted");

        while(turns.size() > 0)
        {
            // Process first turn
            turn_info const& turn = turns.front();

            split_turn_operation<point_type> const& first_op = turn.operations[0];
            split_turn_operation<point_type> const& second_op = turn.operations[1];
            bool do_split = first_op.seg_id.segment_index >= 0
                    && second_op.seg_id.segment_index >= 0;

            if (do_split)
            {
#ifdef BOOST_GEOMETRY_CHECK_SPLIT_RINGS
                ring_type copy = range; // TEMP, for check
#endif
                ring_collection.resize(ring_collection.size() + 1);
                split<ring_tag, Range>::apply(range, ring_collection.back(),
                        turn.operations[0].seg_id, turn.operations[1].seg_id,
                        turn.point);

#ifdef BOOST_GEOMETRY_CHECK_SPLIT_RINGS
                {
                    std::deque<turn_info> splitted_turns;
                    geometry::get_turns
                        <
                            split_calculate_distance_policy
                        >(ring_collection.back(),
                            splitted_turns,
                            detail::get_turns::no_interrupt_policy());

                    if (splitted_turns.size() > 0)
                    {
                        std::cout << "TODO Still intersecting! " << splitted_turns.size() << std::endl;
                        //std::cout << "       " << geometry::wkt(copy) << std::endl;
                        //std::cout << "       " << geometry::wkt(splitted) << std::endl;
                        //report(splitted_turns, "NOT OK");
                        //std::cout << std::endl;
                    }
                }
#endif

            }

            turns.pop_front();


            if (do_split)
            {
                for (typename boost::range_iterator<turns_type>::type
                    rest = boost::begin(turns);
                    rest != boost::end(turns);
                    ++rest)
                {
                    //turn_info copy = turn;
                    if (adapt(rest->operations[0], first_op, second_op)
                        || adapt(rest->operations[1], first_op, second_op))
                    {
                        /**
                        std::cout << " ADAPTED "
                            << copy.operations[0].seg_id.segment_index << "/" << copy.operations[1].seg_id.segment_index
                            << " "
                            << geometry::wkt(copy.point) << std::endl;
                        **/
                    }
                }
            }
            while(turns.size() > 0
                && (turns.front().operations[0].seg_id.segment_index < 0
                    || turns.front().operations[1].seg_id.segment_index < 0))
            {
                turns.pop_front();
            }
        }

        // Add the (possibly untouched) input range
        insert_rings<ring_tag, RingCollection, Range>::apply(ring_collection, range);
    }

public :
    // Copy by value of range is intentional, copy is modified here
    static inline void apply(Range range, RingCollection& ring_collection)
    {
        call(range, ring_collection);
    }
};

template <typename Polygon, typename RingCollection>
struct polygon_split_rings
{
  typedef range_split_rings
        <
            typename ring_type<Polygon>::type,
            RingCollection
        > per_ring;

    static inline void apply(Polygon const& polygon, RingCollection& ring_collection)
    {
        per_ring::apply(exterior_ring(polygon), ring_collection);

        typedef typename boost::range_const_iterator
            <
                typename interior_type<Polygon const>::type
            >::type ring_iterator;

        for (ring_iterator it = boost::begin(interior_rings(polygon));
             it != boost::end(interior_rings(polygon));
             ++it)
        {
            per_ring::apply(*it, ring_collection);
        }
    }
};


}} // namespace detail::split_rings
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag,
    typename Geometry,
    typename RingCollection
>
struct split_rings
{};


template<typename Polygon, typename RingCollection>
struct split_rings<polygon_tag, Polygon, RingCollection>
    : detail::split_rings::polygon_split_rings<Polygon, RingCollection>
{};


template<typename Ring, typename RingCollection>
struct split_rings<ring_tag, Ring, RingCollection>
    : detail::split_rings::range_split_rings<Ring, RingCollection>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template
<
    typename Geometry,
    typename RingCollection
>
inline void split_rings(Geometry const& geometry, RingCollection& out)
{
    concept::check<Geometry const>();
    concept::check<typename boost::range_value<RingCollection>::type>();

    dispatch::split_rings
    <
        typename tag<Geometry>::type,
        Geometry,
        RingCollection
    >::apply(geometry, out);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DETAIL_OVERLAY_SPLIT_RINGS_HPP
