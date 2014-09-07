// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFERED_PIECE_COLLECTION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFERED_PIECE_COLLECTION_HPP

#include <algorithm>
#include <cstddef>
#include <set>
#include <boost/range.hpp>


#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/envelope.hpp>

#include <boost/geometry/strategies/buffer.hpp>

#include <boost/geometry/geometries/ring.hpp>

#include <boost/geometry/algorithms/detail/buffer/buffered_ring.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_policies.hpp>
#include <boost/geometry/algorithms/detail/buffer/get_piece_turns.hpp>
#include <boost/geometry/algorithms/detail/buffer/turn_in_piece_visitor.hpp>
#include <boost/geometry/algorithms/detail/buffer/turn_in_input.hpp>

#include <boost/geometry/algorithms/detail/overlay/add_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/occupation_info.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>

#include <boost/geometry/util/range.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{

enum segment_relation_code
{
    segment_relation_on_left,
    segment_relation_on_right,
    segment_relation_within,
    segment_relation_disjoint
};


template <typename Ring, typename RobustPolicy>
struct buffered_piece_collection
{
    typedef buffered_piece_collection<Ring, RobustPolicy> this_type;

    typedef typename geometry::point_type<Ring>::type point_type;
    typedef typename geometry::coordinate_type<Ring>::type coordinate_type;
    typedef typename geometry::robust_point_type
    <
        point_type,
        RobustPolicy
    >::type robust_point_type;

    typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<point_type>::type
        >::type side_strategy;

    typedef typename geometry::rescale_policy_type
        <
            typename geometry::point_type<Ring>::type
        >::type rescale_policy_type;

    typedef typename geometry::segment_ratio_type
    <
        point_type,
        RobustPolicy
    >::type segment_ratio_type;

    typedef buffer_turn_info
    <
        point_type,
        robust_point_type,
        segment_ratio_type
    > buffer_turn_info_type;

    typedef buffer_turn_operation
    <
        point_type,
        segment_ratio_type
    > buffer_turn_operation_type;

    typedef std::vector<buffer_turn_info_type> turn_vector_type;

    struct robust_turn
    {
        int turn_index;
        int operation_index;
        robust_point_type point;
        segment_identifier seg_id;
        segment_ratio_type fraction;
    };

    struct piece
    {
        strategy::buffer::piece_type type;
        int index;

        int left_index; // points to previous piece
        int right_index; // points to next piece

        // The next two members form together a complete clockwise ring
        // for each piece (with one dupped point)

        // 1: half, part of offsetted_rings
        segment_identifier first_seg_id;
        int last_segment_index; // no segment-identifier - it is the same as first_seg_id
        int offsetted_count;

        // 2: half, not part (future: might be indexed in one vector too)
        std::vector<point_type> helper_segments; // 3 points for segment, 2 points for join - 0 points for flat-end

        // Robust representations
        std::vector<robust_turn> robust_turns; // Used only in rescale_pieces - we might use a map instead
        geometry::model::ring<robust_point_type> robust_ring;
        geometry::model::box<robust_point_type> robust_envelope;
    };

    typedef std::vector<piece> piece_vector_type;

    piece_vector_type m_pieces;
    turn_vector_type m_turns;
    int m_first_piece_index;

    buffered_ring_collection<buffered_ring<Ring> > offsetted_rings; // indexed by multi_index
    std::vector< std::vector < robust_point_type > > robust_offsetted_rings;
    buffered_ring_collection<Ring> traversed_rings;
    segment_identifier current_segment_id;

    RobustPolicy const& m_robust_policy;

    struct redundant_turn
    {
        inline bool operator()(buffer_turn_info_type const& turn) const
        {
            // Erase discarded turns (location not OK) and the turns
            // only used to detect oppositeness.
            return turn.location != location_ok
                || turn.opposite();
        }
    };

    buffered_piece_collection(RobustPolicy const& robust_policy)
        : m_first_piece_index(-1)
        , m_robust_policy(robust_policy)
    {}


#if defined(BOOST_GEOMETRY_BUFFER_ENLARGED_CLUSTERS)
    // Will (most probably) be removed later
    template <typename OccupationMap>
    inline void adapt_mapped_robust_point(OccupationMap const& map,
            buffer_turn_info_type& turn, int distance) const
    {
        for (int x = -distance; x <= distance; x++)
        {
            for (int y = -distance; y <= distance; y++)
            {
                robust_point_type rp = turn.robust_point;
                geometry::set<0>(rp, geometry::get<0>(rp) + x);
                geometry::set<1>(rp, geometry::get<1>(rp) + y);
                if (map.find(rp) != map.end())
                {
                    turn.mapped_robust_point = rp;
                    return;
                }
            }
        }
    }
#endif

    inline void get_occupation(
#if defined(BOOST_GEOMETRY_BUFFER_ENLARGED_CLUSTERS)
        int distance = 0
#endif
    )
    {
        typedef occupation_info<angle_info<robust_point_type, coordinate_type> >
                buffer_occupation_info;

        typedef std::map
        <
            robust_point_type,
            buffer_occupation_info,
            geometry::less<robust_point_type>
        > occupation_map_type;

        occupation_map_type occupation_map;

        // 1: Add all intersection points to occupation map
        typedef typename boost::range_iterator<turn_vector_type>::type
            iterator_type;

        for (iterator_type it = boost::begin(m_turns);
            it != boost::end(m_turns);
            ++it)
        {
            if (it->count_on_offsetted >= 1)
            {
#if defined(BOOST_GEOMETRY_BUFFER_ENLARGED_CLUSTERS)
                if (distance > 0 && ! occupation_map.empty())
                {
                    adapt_mapped_robust_point(occupation_map, *it, distance);
                }
#endif
                occupation_map[it->get_robust_point()].count++;
            }
        }

        // 2: Remove all points from map which has only one
        typename occupation_map_type::iterator it = occupation_map.begin();
        while (it != occupation_map.end())
        {
            if (it->second.count <= 1)
            {
                typename occupation_map_type::iterator to_erase = it;
                ++it;
                occupation_map.erase(to_erase);
            }
            else
            {
                ++it;
            }
        }

        if (occupation_map.empty())
        {
            return;
        }

        // 3: Add vectors (incoming->intersection-point,
        //                 intersection-point -> outgoing)
        //    for all (co-located) points still present in the map

        int index = 0;
        for (iterator_type it = boost::begin(m_turns);
            it != boost::end(m_turns);
            ++it, ++index)
        {
            typename occupation_map_type::iterator mit =
                        occupation_map.find(it->get_robust_point());

            if (mit != occupation_map.end())
            {
                buffer_occupation_info& info = mit->second;
                for (int i = 0; i < 2; i++)
                {
                    add_incoming_and_outgoing_angles(it->get_robust_point(), *it,
                                m_pieces,
                                index, i, it->operations[i].seg_id,
                                info);
                }

                it->count_on_multi++;
            }
        }

#if defined(BOOST_GEOMETRY_BUFFER_ENLARGED_CLUSTERS)
        // X: Check rounding issues
        if (distance == 0)
        {
            for (typename occupation_map_type::const_iterator it = occupation_map.begin();
                it != occupation_map.end(); ++it)
            {
                if (it->second.has_rounding_issues(it->first))
                {
                    if(distance == 0)
                    {
                        get_occupation(distance + 1);
                        return;
                    }
                }
            }
        }
#endif

        // If, in a cluster, one turn is blocked, block them all
        for (typename occupation_map_type::const_iterator it = occupation_map.begin();
            it != occupation_map.end(); ++it)
        {
            typename buffer_occupation_info::turn_vector_type const& turns = it->second.turns;
            bool blocked = false;
            for (std::size_t i = 0; i < turns.size(); i++)
            {
                if (m_turns[turns[i].turn_index].blocked())
                {
                    blocked = true;
                    break;
                }
            }
            if (blocked)
            {
                for (std::size_t i = 0; i < turns.size(); i++)
                {
                    m_turns[turns[i].turn_index].count_on_occupied++;
                }
            }
        }

        // 4: Mark all turns as not selectable as a starting point for traversing
        //    rings. They still can be used to continue already started rings.
        for (typename occupation_map_type::iterator it = occupation_map.begin();
            it != occupation_map.end(); ++it)
        {
            typename buffer_occupation_info::turn_vector_type const& turns = it->second.turns;
            for (std::size_t i = 0; i < turns.size(); i++)
            {
                m_turns[turns[i].turn_index].selectable_start = false;
            }
        }
    }

    inline void classify_turns()
    {
        for (typename boost::range_iterator<turn_vector_type>::type it =
            boost::begin(m_turns); it != boost::end(m_turns); ++it)
        {
            if ( it->count_within > 0
              || it->count_on_occupied > 0 )
            {
                it->location = inside_buffer;
            }
        }
    }

    template <typename Geometry, typename DistanceStrategy>
    inline void check_remaining_points(Geometry const& input_geometry, DistanceStrategy const& distance_strategy)
    {
        int const factor = distance_strategy.factor();

        // This might use partition too (for multi-polygons)

        for (typename boost::range_iterator<turn_vector_type>::type it =
            boost::begin(m_turns); it != boost::end(m_turns); ++it)
        {
            if (it->location == location_ok)
            {
                int code = turn_in_input
                        <
                            typename geometry::tag<Geometry>::type
                        >::apply(it->point, input_geometry);
                if (code * factor == 1)
                {
                    it->location = inside_original;
                }
            }
        }
    }

    inline bool assert_indices_in_robust_rings() const
    {
        geometry::equal_to<robust_point_type> comparator;
        for (typename boost::range_iterator<turn_vector_type const>::type it =
            boost::begin(m_turns); it != boost::end(m_turns); ++it)
        {
            for (int i = 0; i < 2; i++)
            {
                robust_point_type const &p1
                    = m_pieces[it->operations[i].piece_index].robust_ring
                              [it->operations[i].index_in_robust_ring];
                robust_point_type const &p2 = it->robust_point;
                if (! comparator(p1, p2))
                {
                    return false;
                }
            }
        }
        return true;
    }

    inline void rescale_piece_rings()
    {
        for (typename piece_vector_type::iterator it = boost::begin(m_pieces);
            it != boost::end(m_pieces);
            ++it)
        {
            piece& pc = *it;

            pc.offsetted_count = pc.last_segment_index - pc.first_seg_id.segment_index;
            BOOST_ASSERT(pc.offsetted_count >= 0);

            pc.robust_ring.reserve(pc.offsetted_count + pc.helper_segments.size());

            // Add rescaled offsetted segments
            {
                buffered_ring<Ring> const& ring = offsetted_rings[pc.first_seg_id.multi_index];

                typedef typename boost::range_iterator<const buffered_ring<Ring> >::type it_type;
                for (it_type it = boost::begin(ring) + pc.first_seg_id.segment_index;
                    it != boost::begin(ring) + pc.last_segment_index;
                    ++it)
                {
                    robust_point_type point;
                    geometry::recalculate(point, *it, m_robust_policy);
                    pc.robust_ring.push_back(point);
                }
            }

            // Add rescaled helper-segments
            {
                typedef typename std::vector<point_type>::const_iterator it_type;
                for (it_type it = boost::begin(pc.helper_segments);
                    it != boost::end(pc.helper_segments);
                    ++it)
                {
                    robust_point_type point;
                    geometry::recalculate(point, *it, m_robust_policy);
                    pc.robust_ring.push_back(point);
                }
            }

            // Calculate the envelope
            geometry::detail::envelope::envelope_range::apply(pc.robust_ring,
                    pc.robust_envelope);
        }
    }

    inline void insert_rescaled_piece_turns()
    {
        // Add rescaled turn points to corresponding pieces
        // (after this, each turn occurs twice)
        int index = 0;
        for (typename boost::range_iterator<turn_vector_type>::type it =
            boost::begin(m_turns); it != boost::end(m_turns); ++it, ++index)
        {
            geometry::recalculate(it->robust_point, it->point, m_robust_policy);
#if defined(BOOST_GEOMETRY_BUFFER_ENLARGED_CLUSTERS)
            it->mapped_robust_point = it->robust_point;
#endif

            robust_turn turn;
            it->turn_index = index;
            turn.turn_index = index;
            turn.point = it->robust_point;
            for (int i = 0; i < 2; i++)
            {
                turn.operation_index = i;
                turn.seg_id = it->operations[i].seg_id;
                turn.fraction = it->operations[i].fraction;

                piece& pc = m_pieces[it->operations[i].piece_index];
                pc.robust_turns.push_back(turn);

                // Take into account for the box (intersection points should fall inside,
                // but in theory they can be one off because of rounding
                geometry::expand(pc.robust_envelope, it->robust_point);
            }
        }

        // Insert all rescaled turn-points into these rings, to form a
        // reliable integer-based ring. All turns can be compared (inside) to this
        // rings to see if they are inside.

        for (typename piece_vector_type::iterator it = boost::begin(m_pieces);
            it != boost::end(m_pieces);
            ++it)
        {
            piece& pc = *it;
            int piece_segment_index = pc.first_seg_id.segment_index;
            if (! pc.robust_turns.empty())
            {
                if (pc.robust_turns.size() > 1u)
                {
                    std::sort(pc.robust_turns.begin(), pc.robust_turns.end(), buffer_operation_less());
                }
                // Walk through them, in reverse to insert at right index
                int index_offset = pc.robust_turns.size() - 1;
                for (typename std::vector<robust_turn>::const_reverse_iterator
                        rit = pc.robust_turns.rbegin();
                    rit != pc.robust_turns.rend();
                    ++rit, --index_offset)
                {
                    int const index_in_vector = 1 + rit->seg_id.segment_index - piece_segment_index;
                    BOOST_ASSERT
                        (
                            index_in_vector > 0 && index_in_vector < pc.offsetted_count
                        );

                    pc.robust_ring.insert(boost::begin(pc.robust_ring) + index_in_vector, rit->point);
                    pc.offsetted_count++;

                    m_turns[rit->turn_index].operations[rit->operation_index].index_in_robust_ring = index_in_vector + index_offset;
                }
            }
        }

        BOOST_ASSERT(assert_indices_in_robust_rings());
    }

    template <typename Geometry, typename DistanceStrategy>
    inline void get_turns(Geometry const& input_geometry, DistanceStrategy const& distance_strategy)
    {
        rescale_piece_rings();

        {
            // Calculate the turns
            piece_turn_visitor
                <
                    buffered_ring_collection<buffered_ring<Ring> >,
                    turn_vector_type,
                    RobustPolicy
                > visitor(offsetted_rings, m_turns, m_robust_policy, m_pieces.size());

            geometry::partition
                <
                    model::box<robust_point_type>, piece_get_box, piece_ovelaps_box
                >::apply(m_pieces, visitor);
        }

        insert_rescaled_piece_turns();

        {
            // Check if it is inside any of the pieces
            turn_in_piece_visitor
                <
                    turn_vector_type, piece_vector_type
                > visitor(m_turns, m_pieces);

            geometry::partition
                <
                    model::box<robust_point_type>,
                    turn_get_box, turn_ovelaps_box,
                    piece_get_box, piece_ovelaps_box
                >::apply(m_turns, m_pieces, visitor);

        }


        get_occupation();

        classify_turns();

        check_remaining_points(input_geometry, distance_strategy);
    }

    inline void start_new_ring()
    {
        int const n = offsetted_rings.size();
        current_segment_id.source_index = 0;
        current_segment_id.multi_index = n;
        current_segment_id.ring_index = -1;
        current_segment_id.segment_index = 0;

        offsetted_rings.resize(n + 1);

        m_first_piece_index = boost::size(m_pieces);
    }

    inline void finish_ring()
    {
        BOOST_ASSERT(m_first_piece_index != -1);
        if (m_first_piece_index < static_cast<int>(boost::size(m_pieces)))
        {
            // If piece was added
            // Reassign left-of-first and right-of-last
            geometry::range::at(m_pieces, m_first_piece_index).left_index
                                                    = boost::size(m_pieces) - 1;
            geometry::range::back(m_pieces).right_index = m_first_piece_index;
        }
        m_first_piece_index = -1;
    }

    inline int add_point(point_type const& p)
    {
        BOOST_ASSERT
            (
                boost::size(offsetted_rings) > 0
            );

        current_segment_id.segment_index++;
        offsetted_rings.back().push_back(p);
        return offsetted_rings.back().size();
    }

    //-------------------------------------------------------------------------

    inline piece& add_piece(strategy::buffer::piece_type type, bool decrease_segment_index_by_one)
    {
        piece pc;
        pc.type = type;
        pc.index = boost::size(m_pieces);
        pc.first_seg_id = current_segment_id;

        // Assign left/right (for first/last piece per ring they will be re-assigned later)
        pc.left_index = pc.index - 1;
        pc.right_index = pc.index + 1;

        std::size_t const n = boost::size(offsetted_rings.back());
        pc.first_seg_id.segment_index = decrease_segment_index_by_one ? n - 1 : n;

        m_pieces.push_back(pc);
        return m_pieces.back();
    }

    template <typename Range>
    inline void add_piece(strategy::buffer::piece_type type, point_type const& p1, point_type const& p2,
            Range const& range, bool first)
    {
        piece& pc = add_piece(type, ! first);

        // If it follows a non-join (so basically the same piece-type) point b1 should be added.
        // There should be two intersections later and it should be discarded.
        // But for now we need it to calculate intersections
        if (first)
        {
            add_point(range.front());
        }
        pc.last_segment_index = add_point(range.back());

        pc.helper_segments.push_back(range.back());
        pc.helper_segments.push_back(p2);
        pc.helper_segments.push_back(p1);
        pc.helper_segments.push_back(range.front());
    }

    inline void add_piece(strategy::buffer::piece_type type, point_type const& p,
            point_type const& b1, point_type const& b2)
    {
        piece& pc = add_piece(type, false);
        add_point(b1);
        pc.last_segment_index = add_point(b2);
        pc.helper_segments.push_back(b2);
        pc.helper_segments.push_back(p);
        pc.helper_segments.push_back(b1);
    }


    template <typename Range>
    inline piece& add_piece(strategy::buffer::piece_type type, Range const& range, bool decrease_segment_index_by_one)
    {
        piece& pc = add_piece(type, decrease_segment_index_by_one);

        bool first = true;
        int last = offsetted_rings.back().size() + 1;
        for (typename Range::const_iterator it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            bool add = true;
            if (first)
            {
                // Only for very first one, add first. In all other cases it is shared with previous.
                add = offsetted_rings.back().empty();
                first = false;
            }
            if (add)
            {
                last = add_point(*it);
            }

        }

        pc.last_segment_index = last;

        return pc;
    }

    template <typename Range>
    inline void add_piece(strategy::buffer::piece_type type, point_type const& p, Range const& range)
    {
        piece& pc = add_piece(type, range, true);

        if (boost::size(range) > 0)
        {
            pc.helper_segments.push_back(range.back());
            pc.helper_segments.push_back(p);
            pc.helper_segments.push_back(range.front());
        }
    }

    template <typename EndcapStrategy, typename Range>
    inline void add_endcap(EndcapStrategy const& strategy, Range const& range, point_type const& end_point)
    {
        if (range.empty())
        {
            return;
        }
        strategy::buffer::piece_type pt = strategy.get_piece_type();
        if (pt == strategy::buffer::buffered_flat_end)
        {
            // It is flat, should just be added, without helper segments
            add_piece(pt, range, true);
        }
        else
        {
            // Normal case, it has an "inside", helper segments should be added
            add_piece(pt, end_point, range);
        }
    }

    //-------------------------------------------------------------------------

    inline void enrich()
    {
        typedef typename strategy::side::services::default_strategy
        <
            typename cs_tag<Ring>::type
        >::type side_strategy_type;

        enrich_intersection_points<false, false>(m_turns,
                    detail::overlay::operation_union,
                    offsetted_rings, offsetted_rings,
                    m_robust_policy, side_strategy_type());
    }

    // Discards all rings which do have not-OK intersection points only.
    // Those can never be traversed and should not be part of the output.
    inline void discard_rings()
    {
        for (typename boost::range_iterator<turn_vector_type const>::type it =
            boost::begin(m_turns); it != boost::end(m_turns); ++it)
        {
            if (it->location != location_ok)
            {
                offsetted_rings[it->operations[0].seg_id.multi_index].has_discarded_intersections = true;
                offsetted_rings[it->operations[1].seg_id.multi_index].has_discarded_intersections = true;
            }
            else if (! it->both(detail::overlay::operation_union))
            {
                offsetted_rings[it->operations[0].seg_id.multi_index].has_accepted_intersections = true;
                offsetted_rings[it->operations[1].seg_id.multi_index].has_accepted_intersections = true;
            }
        }
    }

    inline void discard_turns()
    {
        m_turns.erase
            (
                std::remove_if(boost::begin(m_turns), boost::end(m_turns),
                                redundant_turn()),
                boost::end(m_turns)
            );

    }

    inline void traverse()
    {
        typedef detail::overlay::traverse
            <
                false, false,
                buffered_ring_collection<buffered_ring<Ring> >, buffered_ring_collection<buffered_ring<Ring > >,
                backtrack_for_buffer
            > traverser;

        traversed_rings.clear();
        traverser::apply(offsetted_rings, offsetted_rings,
                        detail::overlay::operation_union,
                        m_robust_policy, m_turns, traversed_rings);
    }

    inline void reverse()
    {
        for(typename buffered_ring_collection<buffered_ring<Ring> >::iterator it = boost::begin(offsetted_rings);
            it != boost::end(offsetted_rings);
            ++it)
        {
            if (! it->has_intersections())
            {
                std::reverse(it->begin(), it->end());
            }
        }
        for (typename boost::range_iterator<buffered_ring_collection<Ring> >::type
                it = boost::begin(traversed_rings);
                it != boost::end(traversed_rings);
                ++it)
        {
            std::reverse(it->begin(), it->end());
        }

    }

    template <typename GeometryOutput, typename OutputIterator>
    inline OutputIterator assign(OutputIterator out) const
    {
        typedef detail::overlay::ring_properties<point_type> properties;

        std::map<ring_identifier, properties> selected;

        // Select all rings which do not have any self-intersection (other ones should be traversed)
        int index = 0;
        for(typename buffered_ring_collection<buffered_ring<Ring> >::const_iterator it = boost::begin(offsetted_rings);
            it != boost::end(offsetted_rings);
            ++it, ++index)
        {
            if (! it->has_intersections())
            {
                ring_identifier id(0, index, -1);
                selected[id] = properties(*it, true);
            }
        }

        // Select all created rings
        index = 0;
        for (typename boost::range_iterator<buffered_ring_collection<Ring> const>::type
                it = boost::begin(traversed_rings);
                it != boost::end(traversed_rings);
                ++it, ++index)
        {
            ring_identifier id(2, index, -1);
            selected[id] = properties(*it, true);
        }

        detail::overlay::assign_parents(offsetted_rings, traversed_rings, selected, false);
        return detail::overlay::add_rings<GeometryOutput>(selected, offsetted_rings, traversed_rings, out);
    }

};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_BUFFERED_PIECE_COLLECTION_HPP
