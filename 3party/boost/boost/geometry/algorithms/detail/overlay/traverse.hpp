// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP


#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


#if defined(BOOST_GEOMETRY_DEBUG_INTERSECTION) || defined(BOOST_GEOMETRY_OVERLAY_REPORT_WKT)
#  include <string>
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/domains/gis/io/wkt/wkt.hpp>
#endif



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

template <typename Turn, typename Operation>
inline void debug_traverse(Turn const& turn, Operation op, std::string const& header)
{
#ifdef BOOST_GEOMETRY_DEBUG_TRAVERSE
    std::cout << header
        << " at " << op.seg_id
        << " op: " << operation_char(op.operation)
        << " vis: " << visited_char(op.visited)
        << " of:  " << operation_char(turn.operations[0].operation)
        << operation_char(turn.operations[1].operation)
        << std::endl;

    if (boost::contains(header, "Finished"))
    {
        std::cout << std::endl;
    }
#endif
}


template <typename Turns>
inline void clear_visit_info(Turns& turns)
{
    typedef typename boost::range_value<Turns>::type tp_type;

    for (typename boost::range_iterator<Turns>::type
        it = boost::begin(turns);
        it != boost::end(turns);
        ++it)
    {
        for (typename boost::range_iterator
            <
                typename tp_type::container_type
            >::type op_it = boost::begin(it->operations);
            op_it != boost::end(it->operations);
            ++op_it)
        {
            op_it->visited.clear();
        }
        it->discarded = false;
    }
}


template <typename Info, typename Turn>
inline void set_visited_for_continue(Info& info, Turn const& turn)
{
    // On "continue", set "visited" for ALL directions
    if (turn.operation == detail::overlay::operation_continue)
    {
        for (typename boost::range_iterator
            <
                typename Info::container_type
            >::type it = boost::begin(info.operations);
            it != boost::end(info.operations);
            ++it)
        {
            if (it->visited.none())
            {
                it->visited.set_visited();
            }
        }
    }
}


template
<
    bool Reverse1, bool Reverse2,
    typename GeometryOut,
    typename G1,
    typename G2,
    typename Turns,
    typename IntersectionInfo
>
inline bool assign_next_ip(G1 const& g1, G2 const& g2,
            Turns& turns,
            typename boost::range_iterator<Turns>::type& ip,
            GeometryOut& current_output,
            IntersectionInfo& info,
            segment_identifier& seg_id)
{
    info.visited.set_visited();
    set_visited_for_continue(*ip, info);

    // If there is no next IP on this segment
    if (info.enriched.next_ip_index < 0)
    {
        if (info.enriched.travels_to_vertex_index < 0 || info.enriched.travels_to_ip_index < 0) return false;

        BOOST_ASSERT(info.enriched.travels_to_vertex_index >= 0);
        BOOST_ASSERT(info.enriched.travels_to_ip_index >= 0);

        if (info.seg_id.source_index == 0)
        {
            geometry::copy_segments<Reverse1>(g1, info.seg_id,
                    info.enriched.travels_to_vertex_index,
                    current_output);
        }
        else
        {
            geometry::copy_segments<Reverse2>(g2, info.seg_id,
                    info.enriched.travels_to_vertex_index,
                    current_output);
        }
        seg_id = info.seg_id;
        ip = boost::begin(turns) + info.enriched.travels_to_ip_index;
    }
    else
    {
        ip = boost::begin(turns) + info.enriched.next_ip_index;
        seg_id = info.seg_id;
    }

    geometry::append(current_output, ip->point);
    return true;
}


inline bool select_source(operation_type operation, int source1, int source2)
{
    return (operation == operation_intersection && source1 != source2)
        || (operation == operation_union && source1 == source2)
        ;
}


template
<
    typename Turn,
    typename Iterator
>
inline bool select_next_ip(operation_type operation,
            Turn& turn,
            segment_identifier const& seg_id,
            Iterator& selected)
{
    if (turn.discarded)
    {
        return false;
    }
    bool has_tp = false;
    selected = boost::end(turn.operations);
    for (Iterator it = boost::begin(turn.operations);
        it != boost::end(turn.operations);
        ++it)
    {
        if (it->visited.started())
        {
            selected = it;
            //std::cout << " RETURN";
            return true;
        }

        // In some cases there are two alternatives.
        // For "ii", take the other one (alternate)
        //           UNLESS the other one is already visited
        // For "uu", take the same one (see above);
        // For "cc", take either one, but if there is a starting one,
        //           take that one.
        if (   (it->operation == operation_continue
                && (! has_tp || it->visited.started()
                    )
                )
            || (it->operation == operation
                && ! it->visited.finished()
                && (! has_tp
                    || select_source(operation,
                            it->seg_id.source_index, seg_id.source_index)
                    )
                )
            )
        {
            selected = it;
            debug_traverse(turn, *it, " Candidate");
            has_tp = true;
        }
    }

    if (has_tp)
    {
       debug_traverse(turn, *selected, "  Accepted");
    }


    return has_tp;
}



template
<
    typename Rings,
    typename Turns,
    typename Operation,
    typename Geometry1,
    typename Geometry2
>
inline void backtrack(std::size_t size_at_start, bool& fail,
            Rings& rings, typename boost::range_value<Rings>::type& ring,
            Turns& turns, Operation& operation,

#ifdef BOOST_GEOMETRY_OVERLAY_REPORT_WKT
            std::string const& reason,
            Geometry1 const& geometry1,
            Geometry2 const& geometry2
#else
            std::string const& reason,
            Geometry1 const& ,
            Geometry2 const&
#endif
            )
{
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    std::cout << " REJECT " << reason << std::endl;
#endif
    fail = true;

    // Make bad output clean
    rings.resize(size_at_start);
    ring.clear();

    // Reject this as a starting point
    operation.visited.set_rejected();

    // And clear all visit info
    clear_visit_info(turns);

    /***
    int c = 0;
    for (int i = 0; i < turns.size(); i++)
    {
        for (int j = 0; j < 2; j++)
        {
            if (turns[i].operations[j].visited.rejected())
            {
                c++;
            }
        }
    }
    std::cout << "BACKTRACK (" << reason << " )"
        << " " << c << " of " << turns.size() << " rejected"
        << std::endl;
    ***/



#ifdef BOOST_GEOMETRY_OVERLAY_REPORT_WKT
    std::cout << " BT (" << reason << " )";
    std::cout
        << geometry::wkt(geometry1) << std::endl
        << geometry::wkt(geometry2) << std::endl;
#endif

}

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


/*!
    \brief Traverses through intersection points / geometries
    \ingroup overlay
 */
template
<
    bool Reverse1, bool Reverse2,
    typename Geometry1,
    typename Geometry2,
    typename Turns,
    typename Rings
>
inline void traverse(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            detail::overlay::operation_type operation,
            Turns& turns, Rings& rings)
{
    typedef typename boost::range_iterator<Turns>::type turn_iterator;
    typedef typename boost::range_value<Turns>::type turn_type;
    typedef typename boost::range_iterator
        <
            typename turn_type::container_type
        >::type turn_operation_iterator_type;

    std::size_t size_at_start = boost::size(rings);

    bool fail = false;
    do
    {
        fail = false;
        // Iterate through all unvisited points
        for (turn_iterator it = boost::begin(turns);
            ! fail && it != boost::end(turns);
            ++it)
        {
            // Skip discarded ones
            if (! (it->is_discarded() || it->blocked()))
            {
                for (turn_operation_iterator_type iit = boost::begin(it->operations);
                    ! fail && iit != boost::end(it->operations);
                    ++iit)
                {
                    if (iit->visited.none()
                        && ! iit->visited.rejected()
                        && (iit->operation == operation
                            || iit->operation == detail::overlay::operation_continue)
                        )
                    {
                        set_visited_for_continue(*it, *iit);

                        typename boost::range_value<Rings>::type current_output;
                        geometry::append(current_output, it->point);

                        turn_iterator current = it;
                        turn_operation_iterator_type current_iit = iit;
                        segment_identifier current_seg_id;

                        if (! detail::overlay::assign_next_ip<Reverse1, Reverse2>(
                                    geometry1, geometry2,
                                    turns,
                                    current, current_output,
                                    *iit, current_seg_id))
                        {
                            detail::overlay::backtrack(
                                size_at_start, fail,
                                rings, current_output, turns, *current_iit,
                                "No next IP",
                                geometry1, geometry2);
                        }

                        if (! detail::overlay::select_next_ip(
                                        operation,
                                        *current,
                                        current_seg_id,
                                        current_iit))
                        {
                            detail::overlay::backtrack(
                                size_at_start, fail,
                                rings, current_output, turns, *iit,
                                "Dead end at start",
                                geometry1, geometry2);
                        }
                        else
                        {

                            iit->visited.set_started();
                            detail::overlay::debug_traverse(*it, *iit, "-> Started");
                            detail::overlay::debug_traverse(*current, *current_iit, "Selected  ");


                            unsigned int i = 0;

                            while (current_iit != iit && ! fail)
                            {
                                if (current_iit->visited.visited())
                                {
                                    // It visits a visited node again, without passing the start node.
                                    // This makes it suspicious for endless loops
                                    detail::overlay::backtrack(
                                        size_at_start, fail,
                                        rings,  current_output, turns, *iit,
                                        "Visit again",
                                        geometry1, geometry2);
                                }
                                else
                                {


                                    // We assume clockwise polygons only, non self-intersecting, closed.
                                    // However, the input might be different, and checking validity
                                    // is up to the library user.

                                    // Therefore we make here some sanity checks. If the input
                                    // violates the assumptions, the output polygon will not be correct
                                    // but the routine will stop and output the current polygon, and
                                    // will continue with the next one.

                                    // Below three reasons to stop.
                                    detail::overlay::assign_next_ip<Reverse1, Reverse2>(
                                        geometry1, geometry2,
                                        turns, current, current_output,
                                        *current_iit, current_seg_id);

                                    if (! detail::overlay::select_next_ip(
                                                operation,
                                                *current,
                                                current_seg_id,
                                                current_iit))
                                    {
                                        // Should not occur in valid (non-self-intersecting) polygons
                                        // Should not occur in self-intersecting polygons without spikes
                                        // Might occur in polygons with spikes
                                        detail::overlay::backtrack(
                                            size_at_start, fail,
                                            rings,  current_output, turns, *iit,
                                            "Dead end",
                                            geometry1, geometry2);
                                    }
                                    detail::overlay::debug_traverse(*current, *current_iit, "Selected  ");

                                    if (i++ > 2 + 2 * turns.size())
                                    {
                                        // Sanity check: there may be never more loops
                                        // than turn points.
                                        // Turn points marked as "ii" can be visited twice.
                                        detail::overlay::backtrack(
                                            size_at_start, fail,
                                            rings,  current_output, turns, *iit,
                                            "Endless loop",
                                            geometry1, geometry2);
                                    }
                                }
                            }

                            if (! fail)
                            {
                                iit->visited.set_finished();
                                detail::overlay::debug_traverse(*current, *iit, "->Finished");
                                rings.push_back(current_output);
                            }
                        }
                    }
                }
            }
        }
    } while (fail);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_TRAVERSE_HPP
