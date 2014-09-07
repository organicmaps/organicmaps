// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP

#include <algorithm>
#include <boost/range.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/iterators/closing_iterator.hpp>

#include <boost/geometry/algorithms/detail/get_left_turns.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Point, typename T>
struct angle_info
{
    typedef T angle_type;
    typedef Point point_type;

    segment_identifier seg_id;
    int turn_index;
    int operation_index;
    Point intersection_point;
    Point point; // either incoming or outgoing point
    bool incoming;
    bool blocked;
    bool included;

    inline angle_info()
        : blocked(false)
        , included(false)
    {}
};

template <typename AngleInfo>
class occupation_info
{
public :
    typedef std::vector<AngleInfo> collection_type;
    typedef std::vector
        <
            detail::left_turns::turn_angle_info<typename AngleInfo::point_type>
        > turn_vector_type;

    collection_type angles; // each turn splitted in incoming/outgoing vectors
    turn_vector_type turns;
    int count;


    inline occupation_info()
        : count(0)
    {}

    template <typename RobustPoint>
    inline void add(RobustPoint const& incoming_point,
                    RobustPoint const& outgoing_point,
                    RobustPoint const& intersection_point,
                    int turn_index, int operation_index,
                    segment_identifier const& seg_id)
    {
        geometry::equal_to<RobustPoint> comparator;
        if (comparator(incoming_point, intersection_point))
        {
            return;
        }
        if (comparator(outgoing_point, intersection_point))
        {
            return;
        }

        AngleInfo info;
        info.seg_id = seg_id;
        info.turn_index = turn_index;
        info.operation_index = operation_index;
        info.intersection_point = intersection_point;

        {
            info.point = incoming_point;
            info.incoming = true;
            angles.push_back(info);
        }
        {
            info.point = outgoing_point;
            info.incoming = false;
            angles.push_back(info);
        }
        detail::left_turns::turn_angle_info<typename AngleInfo::point_type> turn(seg_id, incoming_point, outgoing_point);
        turn.turn_index = turn_index;
        turns.push_back(turn);
    }

    template <typename RobustPoint>
    inline void get_left_turns(RobustPoint const& origin,
                    std::vector<detail::left_turns::left_turn>& turns_to_keep)
    {
        // Sort on angle
        std::sort(angles.begin(), angles.end(),
                detail::left_turns::angle_less<typename AngleInfo::point_type>(origin));

        // Block all turns on the right side of any turn
        detail::left_turns::block_turns_on_right_sides(turns, angles);

        detail::left_turns::get_left_turns(angles, origin, turns_to_keep);
    }

    template <typename RobustPoint>
    inline bool has_rounding_issues(RobustPoint const& origin) const
    {
        return detail::left_turns::has_rounding_issues(angles, origin);
    }
};

template<typename Pieces>
inline void move_index(Pieces const& pieces, int& index, int& piece_index, int direction)
{
    BOOST_ASSERT(direction == 1 || direction == -1);
    BOOST_ASSERT(piece_index >= 0 && piece_index < static_cast<int>(boost::size(pieces)) );
    BOOST_ASSERT(index >= 0 && index < static_cast<int>(boost::size(pieces[piece_index].robust_ring)));

    index += direction;
    if (direction == -1 && index < 0)
    {
        piece_index--;
        if (piece_index < 0)
        {
            piece_index = boost::size(pieces) - 1;
        }
        index = boost::size(pieces[piece_index].robust_ring) - 1;
    }
    if (direction == 1
        && index >= static_cast<int>(boost::size(pieces[piece_index].robust_ring)))
    {
        piece_index++;
        if (piece_index >= static_cast<int>(boost::size(pieces)))
        {
            piece_index = 0;
        }
        index = 0;
    }
}


template
<
    typename RobustPoint,
    typename Turn,
    typename Pieces,
    typename Info
>
inline void add_incoming_and_outgoing_angles(
                RobustPoint const& intersection_point, // rescaled
                Turn const& turn,
                Pieces const& pieces, // using rescaled offsets of it
                int turn_index,
                int operation_index,
                segment_identifier seg_id,
                Info& info)
{
    segment_identifier real_seg_id = seg_id;
    geometry::equal_to<RobustPoint> comparator;

    // Move backward and forward
    RobustPoint direction_points[2];
    for (int i = 0; i < 2; i++)
    {
        int index = turn.operations[operation_index].index_in_robust_ring;
        int piece_index = turn.operations[operation_index].piece_index;
        while(comparator(pieces[piece_index].robust_ring[index], intersection_point))
        {
            move_index(pieces, index, piece_index, i == 0 ? -1 : 1);
        }
        direction_points[i] = pieces[piece_index].robust_ring[index];
    }

    info.add(direction_points[0], direction_points[1], intersection_point,
        turn_index, operation_index, real_seg_id);
}


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OCCUPATION_INFO_HPP
