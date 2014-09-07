// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_GET_PIECE_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_GET_PIECE_TURNS_HPP

#include <boost/range.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/detail/disjoint/box_box.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace buffer
{


struct piece_get_box
{
    template <typename Box, typename Piece>
    static inline void apply(Box& total, Piece const& piece)
    {
        geometry::expand(total, piece.robust_envelope);
    }
};

struct piece_ovelaps_box
{
    template <typename Box, typename Piece>
    static inline bool apply(Box const& box, Piece const& piece)
    {
        return ! geometry::detail::disjoint::disjoint_box_box(box, piece.robust_envelope);
    }
};

template
<
    typename Rings,
    typename Turns,
    typename RobustPolicy
>
class piece_turn_visitor
{
    Rings const& m_rings;
    Turns& m_turns;
    RobustPolicy const& m_robust_policy;
    int m_last_piece_index;

    template <typename Piece>
    inline bool is_adjacent(Piece const& piece1, Piece const& piece2) const
    {
        if (piece1.first_seg_id.multi_index != piece2.first_seg_id.multi_index)
        {
            return false;
        }

        if (std::abs(piece1.index - piece2.index) == 1)
        {
            return true;
        }

        return (piece1.index == 0 && piece2.index == m_last_piece_index)
            || (piece1.index == m_last_piece_index && piece2.index == 0)
            ;
    }

    template <typename Range, typename Iterator>
    inline void move_to_next_point(Range const& range, Iterator& next) const
    {
        ++next;
        if (next == boost::end(range))
        {
            next = boost::begin(range) + 1;
        }
    }

    template <typename Range, typename Iterator>
    inline Iterator next_point(Range const& range, Iterator it) const
    {
        Iterator result = it;
        move_to_next_point(range, result);
        // TODO: we could use either piece-boundaries, or comparison with
        // robust points, to check if the point equals the last one
        while(geometry::equals(*it, *result))
        {
            move_to_next_point(range, result);
        }
        return result;
    }

    template <typename Piece>
    inline void calculate_turns(Piece const& piece1, Piece const& piece2)
    {
        typedef typename boost::range_value<Rings const>::type ring_type;
        typedef typename boost::range_value<Turns const>::type turn_type;
        typedef typename boost::range_iterator<ring_type const>::type iterator;

        segment_identifier seg_id1 = piece1.first_seg_id;
        segment_identifier seg_id2 = piece2.first_seg_id;

        if (seg_id1.segment_index < 0 || seg_id2.segment_index < 0)
        {
            return;
        }

        ring_type const& ring1 = m_rings[seg_id1.multi_index];
        iterator it1_first = boost::begin(ring1) + seg_id1.segment_index;
        iterator it1_last = boost::begin(ring1) + piece1.last_segment_index;

        ring_type const& ring2 = m_rings[seg_id2.multi_index];
        iterator it2_first = boost::begin(ring2) + seg_id2.segment_index;
        iterator it2_last = boost::begin(ring2) + piece2.last_segment_index;

        turn_type the_model;
        the_model.operations[0].piece_index = piece1.index;
        the_model.operations[0].seg_id = piece1.first_seg_id;

        iterator it1 = it1_first;
        for (iterator prev1 = it1++;
                it1 != it1_last;
                prev1 = it1++, the_model.operations[0].seg_id.segment_index++)
        {
            the_model.operations[1].piece_index = piece2.index;
            the_model.operations[1].seg_id = piece2.first_seg_id;

            iterator next1 = next_point(ring1, it1);

            iterator it2 = it2_first;
            for (iterator prev2 = it2++;
                    it2 != it2_last;
                    prev2 = it2++, the_model.operations[1].seg_id.segment_index++)
            {
                // Revert (this is used more often - should be common function TODO)
                the_model.operations[0].other_id = the_model.operations[1].seg_id;
                the_model.operations[1].other_id = the_model.operations[0].seg_id;

                iterator next2 = next_point(ring2, it2);

                // TODO: internally get_turn_info calculates robust points.
                // But they are already calculated.
                // We should be able to use them.
                // this means passing them to this visitor,
                // and iterating in sync with them...
                typedef detail::overlay::get_turn_info
                    <
                        detail::overlay::assign_null_policy
                    > turn_policy;

                turn_policy::apply(*prev1, *it1, *next1,
                                    *prev2, *it2, *next2,
                                    false, false, false, false,
                                    the_model, m_robust_policy,
                                    std::back_inserter(m_turns));
            }
        }
    }

public:

    piece_turn_visitor(Rings const& ring_collection,
            Turns& turns,
            RobustPolicy const& robust_policy,
            int last_piece_index)
        : m_rings(ring_collection)
        , m_turns(turns)
        , m_robust_policy(robust_policy)
        , m_last_piece_index(last_piece_index)
    {}

    template <typename Piece>
    inline void apply(Piece const& piece1, Piece const& piece2,
                    bool first = true)
    {
        boost::ignore_unused_variable_warning(first);
        if ( is_adjacent(piece1, piece2)
          || detail::disjoint::disjoint_box_box(piece1.robust_envelope,
                    piece2.robust_envelope))
        {
            return;
        }
        calculate_turns(piece1, piece2);
    }
};


}} // namespace detail::buffer
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_BUFFER_GET_PIECE_TURNS_HPP
