// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICH_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICH_HPP

#include <cstddef>
#include <algorithm>
#include <map>
#include <set>
#include <vector>

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
#  include <iostream>
#  include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#  include <boost/geometry/io/wkt/wkt.hpp>
#  define BOOST_GEOMETRY_DEBUG_IDENTIFIER
#endif

#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/ring_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segment_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/handle_tangencies.hpp>
#include <boost/geometry/policies/robustness/robust_type.hpp>
#include <boost/geometry/strategies/side.hpp>
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
#  include <boost/geometry/algorithms/detail/overlay/check_enrich.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

// Wraps "turn_operation" from turn_info.hpp,
// giving it extra information
template <typename TurnOperation>
struct indexed_turn_operation
{
    typedef TurnOperation type;

    std::size_t turn_index;
    std::size_t operation_index;
    bool discarded;
    // use pointers to avoid copies, const& is not possible because of usage in vector
    segment_identifier const* other_seg_id; // segment id of other segment of intersection of two segments
    TurnOperation const* subject;

    inline indexed_turn_operation(std::size_t ti, std::size_t oi,
                TurnOperation const& sub,
                segment_identifier const& oid)
        : turn_index(ti)
        , operation_index(oi)
        , discarded(false)
        , other_seg_id(&oid)
        , subject(boost::addressof(sub))
    {}

};

template <typename IndexedTurnOperation>
struct remove_discarded
{
    inline bool operator()(IndexedTurnOperation const& operation) const
    {
        return operation.discarded;
    }
};


template
<
    typename TurnPoints,
    typename Indexed,
    typename Geometry1, typename Geometry2,
    typename RobustPolicy,
    bool Reverse1, bool Reverse2,
    typename Strategy
>
struct sort_on_segment_and_ratio
{
    inline sort_on_segment_and_ratio(TurnPoints const& turn_points
            , Geometry1 const& geometry1
            , Geometry2 const& geometry2
            , RobustPolicy const& robust_policy
            , Strategy const& strategy
            , bool* clustered)
        : m_turn_points(turn_points)
        , m_geometry1(geometry1)
        , m_geometry2(geometry2)
        , m_robust_policy(robust_policy)
        , m_strategy(strategy)
        , m_clustered(clustered)
    {
    }

private :

    TurnPoints const& m_turn_points;
    Geometry1 const& m_geometry1;
    Geometry2 const& m_geometry2;
    RobustPolicy const& m_robust_policy;
    Strategy const& m_strategy;
    mutable bool* m_clustered;

    typedef typename geometry::point_type<Geometry1>::type point_type;

    inline bool default_order(Indexed const& left, Indexed const& right) const
    {
        // We've nothing to sort on. Take the indexes
        return left.turn_index < right.turn_index;
    }

    inline bool consider_relative_order(Indexed const& left,
                    Indexed const& right) const
    {
        point_type pi, pj, ri, rj, si, sj;

        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            left.subject->seg_id,
            pi, pj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            *left.other_seg_id,
            ri, rj);
        geometry::copy_segment_points<Reverse1, Reverse2>(m_geometry1, m_geometry2,
            *right.other_seg_id,
            si, sj);

        typedef typename strategy::side::services::default_strategy
            <
                typename cs_tag<point_type>::type
            >::type strategy;

        int const side_rj_p = strategy::apply(pi, pj, rj);
        int const side_sj_p = strategy::apply(pi, pj, sj);

        // Put the one turning left (1; right == -1) as last
        if (side_rj_p != side_sj_p)
        {
            return side_rj_p < side_sj_p;
        }

        int const side_sj_r = strategy::apply(ri, rj, sj);
        int const side_rj_s = strategy::apply(si, sj, rj);

        // If they both turn left: the most left as last
        // If they both turn right: this is not relevant, but take also here most left
        if (side_rj_s != side_sj_r)
        {
            return side_rj_s < side_sj_r;
        }

        return default_order(left, right);
    }

public :

    // Note that left/right do NOT correspond to m_geometry1/m_geometry2
    // but to the "indexed_turn_operation"
    inline bool operator()(Indexed const& left, Indexed const& right) const
    {
        if (! (left.subject->seg_id == right.subject->seg_id))
        {
            return left.subject->seg_id < right.subject->seg_id;
        }

        // Both left and right are located on the SAME segment.

        if (! (left.subject->fraction == right.subject->fraction))
        {
            return left.subject->fraction < right.subject->fraction;
        }


        // First check "real" intersection (crosses)
        // -> distance zero due to precision, solve it by sorting
        if (m_turn_points[left.turn_index].method == method_crosses
            && m_turn_points[right.turn_index].method == method_crosses)
        {
            return consider_relative_order(left, right);
        }

        // If that is not the case, cluster it later on.
        // Indicate that this is necessary.
        *m_clustered = true;

        return default_order(left, right);
    }
};


template<typename Turns, typename Operations>
inline void update_discarded(Turns& turn_points, Operations& operations)
{
    // Vice-versa, set discarded to true for discarded operations;
    // AND set discarded points to true
    for (typename boost::range_iterator<Operations>::type it = boost::begin(operations);
         it != boost::end(operations);
         ++it)
    {
        if (turn_points[it->turn_index].discarded)
        {
            it->discarded = true;
        }
        else if (it->discarded)
        {
            turn_points[it->turn_index].discarded = true;
        }
    }
}


// Sorts IP-s of this ring on segment-identifier, and if on same segment,
//  on distance.
// Then assigns for each IP which is the next IP on this segment,
// plus the vertex-index to travel to, plus the next IP
// (might be on another segment)
template
<
    typename IndexType,
    bool Reverse1, bool Reverse2,
    typename Container,
    typename TurnPoints,
    typename Geometry1, typename Geometry2,
    typename RobustPolicy,
    typename Strategy
>
inline void enrich_sort(Container& operations,
            TurnPoints& turn_points,
            operation_type for_operation,
            Geometry1 const& geometry1, Geometry2 const& geometry2,
            RobustPolicy const& robust_policy,
            Strategy const& strategy)
{
    typedef typename IndexType::type operations_type;

    bool clustered = false;
    std::sort(boost::begin(operations),
                boost::end(operations),
                sort_on_segment_and_ratio
                    <
                        TurnPoints,
                        IndexType,
                        Geometry1, Geometry2,
                        RobustPolicy,
                        Reverse1, Reverse2,
                        Strategy
                    >(turn_points, geometry1, geometry2, robust_policy, strategy, &clustered));

    // DONT'T discard xx / (for union) ix / ii / (for intersection) ux / uu here
    // It would give way to "lonely" ui turn points, traveling all
    // the way round. See #105

    if (clustered)
    {
        typedef typename boost::range_iterator<Container>::type nc_iterator;
        nc_iterator it = boost::begin(operations);
        nc_iterator begin_cluster = boost::end(operations);
        for (nc_iterator prev = it++;
            it != boost::end(operations);
            prev = it++)
        {
            operations_type& prev_op = turn_points[prev->turn_index]
                .operations[prev->operation_index];
            operations_type& op = turn_points[it->turn_index]
                .operations[it->operation_index];

            if (prev_op.seg_id == op.seg_id
                && (turn_points[prev->turn_index].method != method_crosses
                    || turn_points[it->turn_index].method != method_crosses)
                && prev_op.fraction == op.fraction)
            {
                if (begin_cluster == boost::end(operations))
                {
                    begin_cluster = prev;
                }
            }
            else if (begin_cluster != boost::end(operations))
            {
                handle_cluster<IndexType, Reverse1, Reverse2>(begin_cluster, it, turn_points,
                        for_operation, geometry1, geometry2, robust_policy, strategy);
                begin_cluster = boost::end(operations);
            }
        }
        if (begin_cluster != boost::end(operations))
        {
            handle_cluster<IndexType, Reverse1, Reverse2>(begin_cluster, it, turn_points,
                    for_operation, geometry1, geometry2, robust_policy, strategy);
        }
    }

    update_discarded(turn_points, operations);
}


template
<
    typename IndexType,
    typename Container,
    typename TurnPoints
>
inline void enrich_discard(Container& operations, TurnPoints& turn_points)
{
    update_discarded(turn_points, operations);

    // Then delete discarded operations from vector
    remove_discarded<IndexType> predicate;
    operations.erase(
            std::remove_if(boost::begin(operations),
                    boost::end(operations),
                    predicate),
            boost::end(operations));
}

template
<
    typename IndexType,
    typename Container,
    typename TurnPoints,
    typename Geometry1,
    typename Geometry2,
    typename Strategy
>
inline void enrich_assign(Container& operations,
            TurnPoints& turn_points,
            operation_type ,
            Geometry1 const& , Geometry2 const& ,
            Strategy const& )
{
    typedef typename IndexType::type operations_type;
    typedef typename boost::range_iterator<Container const>::type iterator_type;


    if (operations.size() > 0)
    {
        // Assign travel-to-vertex/ip index for each turning point.
        // Because IP's are circular, PREV starts at the very last one,
        // being assigned from the first one.
        // "next ip on same segment" should not be considered circular.
        bool first = true;
        iterator_type it = boost::begin(operations);
        for (iterator_type prev = it + (boost::size(operations) - 1);
             it != boost::end(operations);
             prev = it++)
        {
            operations_type& prev_op
                    = turn_points[prev->turn_index].operations[prev->operation_index];
            operations_type& op
                    = turn_points[it->turn_index].operations[it->operation_index];

            prev_op.enriched.travels_to_ip_index
                    = static_cast<signed_size_type>(it->turn_index);
            prev_op.enriched.travels_to_vertex_index
                    = it->subject->seg_id.segment_index;

            if (! first
                && prev_op.seg_id.segment_index == op.seg_id.segment_index)
            {
                prev_op.enriched.next_ip_index = static_cast<signed_size_type>(it->turn_index);
            }
            first = false;
        }
    }

    // DEBUG
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    {
        for (iterator_type it = boost::begin(operations);
             it != boost::end(operations);
             ++it)
        {
            operations_type& op = turn_points[it->turn_index]
                .operations[it->operation_index];

            std::cout << it->turn_index
                << " meth: " << method_char(turn_points[it->turn_index].method)
                << " seg: " << op.seg_id
                << " dst: " << op.fraction // needs define
                << " op: " << operation_char(turn_points[it->turn_index].operations[0].operation)
                << operation_char(turn_points[it->turn_index].operations[1].operation)
                << " dsc: " << (turn_points[it->turn_index].discarded ? "T" : "F")
                << " ->vtx " << op.enriched.travels_to_vertex_index
                << " ->ip " << op.enriched.travels_to_ip_index
                << " ->nxt ip " << op.enriched.next_ip_index
                //<< " vis: " << visited_char(op.visited)
                << std::endl;
                ;
        }
    }
#endif
    // END DEBUG

}


template <typename IndexedType, typename TurnPoints, typename MappedVector>
inline void create_map(TurnPoints const& turn_points, MappedVector& mapped_vector)
{
    typedef typename boost::range_value<TurnPoints>::type turn_point_type;
    typedef typename turn_point_type::container_type container_type;

    std::size_t index = 0;
    for (typename boost::range_iterator<TurnPoints const>::type
            it = boost::begin(turn_points);
         it != boost::end(turn_points);
         ++it, ++index)
    {
        // Add operations on this ring, but skip discarded ones
        if (! it->discarded)
        {
            std::size_t op_index = 0;
            for (typename boost::range_iterator<container_type const>::type
                    op_it = boost::begin(it->operations);
                op_it != boost::end(it->operations);
                ++op_it, ++op_index)
            {
                // We should NOT skip blocked operations here
                // because they can be relevant for "the other side"
                // NOT if (op_it->operation != operation_blocked)

                ring_identifier ring_id
                    (
                        op_it->seg_id.source_index,
                        op_it->seg_id.multi_index,
                        op_it->seg_id.ring_index
                    );
                mapped_vector[ring_id].push_back
                    (
                        IndexedType(index, op_index, *op_it,
                            it->operations[1 - op_index].seg_id)
                    );
            }
        }
    }
}


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL



/*!
\brief All intersection points are enriched with successor information
\ingroup overlay
\tparam TurnPoints type of intersection container
            (e.g. vector of "intersection/turn point"'s)
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam Strategy side strategy type
\param turn_points container containing intersectionpoints
\param for_operation operation_type (union or intersection)
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param robust_policy policy to handle robustness issues
\param strategy strategy
 */
template
<
    bool Reverse1, bool Reverse2,
    typename TurnPoints,
    typename Geometry1, typename Geometry2,
    typename RobustPolicy,
    typename Strategy
>
inline void enrich_intersection_points(TurnPoints& turn_points,
    detail::overlay::operation_type for_operation,
    Geometry1 const& geometry1, Geometry2 const& geometry2,
    RobustPolicy const& robust_policy,
    Strategy const& strategy)
{
    typedef typename boost::range_value<TurnPoints>::type turn_point_type;
    typedef typename turn_point_type::turn_operation_type turn_operation_type;
    typedef detail::overlay::indexed_turn_operation
        <
            turn_operation_type
        > indexed_turn_operation;

    typedef std::map
        <
            ring_identifier,
            std::vector<indexed_turn_operation>
        > mapped_vector_type;

    // DISCARD ALL UU
    // #76 is the reason that this is necessary...
    // With uu, at all points there is the risk that rings are being traversed twice or more.
    // Without uu, all rings having only uu will be untouched and gathered by assemble
    for (typename boost::range_iterator<TurnPoints>::type
            it = boost::begin(turn_points);
         it != boost::end(turn_points);
         ++it)
    {
        if (it->both(detail::overlay::operation_union))
        {
            it->discarded = true;
        }
        if (it->both(detail::overlay::operation_none))
        {
            it->discarded = true;
        }
    }


    // Create a map of vectors of indexed operation-types to be able
    // to sort intersection points PER RING
    mapped_vector_type mapped_vector;

    detail::overlay::create_map<indexed_turn_operation>(turn_points, mapped_vector);


    // No const-iterator; contents of mapped copy is temporary,
    // and changed by enrich
    for (typename mapped_vector_type::iterator mit
        = mapped_vector.begin();
        mit != mapped_vector.end();
        ++mit)
    {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    std::cout << "ENRICH-sort Ring "
        << mit->first << std::endl;
#endif
        detail::overlay::enrich_sort<indexed_turn_operation, Reverse1, Reverse2>(mit->second, turn_points, for_operation,
                    geometry1, geometry2, robust_policy, strategy);
    }

    for (typename mapped_vector_type::iterator mit
        = mapped_vector.begin();
        mit != mapped_vector.end();
        ++mit)
    {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    std::cout << "ENRICH-discard Ring "
        << mit->first << std::endl;
#endif
        detail::overlay::enrich_discard<indexed_turn_operation>(mit->second, turn_points);
    }

    for (typename mapped_vector_type::iterator mit
        = mapped_vector.begin();
        mit != mapped_vector.end();
        ++mit)
    {
#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    std::cout << "ENRICH-assign Ring "
        << mit->first << std::endl;
#endif
        detail::overlay::enrich_assign<indexed_turn_operation>(mit->second, turn_points, for_operation,
                    geometry1, geometry2, strategy);
    }

#ifdef BOOST_GEOMETRY_DEBUG_ENRICH
    //detail::overlay::check_graph(turn_points, for_operation);
#endif

}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_ENRICH_HPP
