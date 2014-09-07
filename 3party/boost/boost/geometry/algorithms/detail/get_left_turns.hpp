// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_identifier.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/iterators/closing_iterator.hpp>
#include <boost/geometry/iterators/ever_circling_iterator.hpp>
#include <boost/geometry/strategies/side.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

// TODO: move this to /util/
template <typename T>
inline std::pair<T, T> ordered_pair(T const& first, T const& second)
{
    return first < second ? std::make_pair(first, second) : std::make_pair(second, first);
}

namespace left_turns
{

template <typename Point>
struct turn_angle_info
{
    segment_identifier seg_id;
    int turn_index;
    Point points[2];

    turn_angle_info(segment_identifier const& id, Point const& from, Point const& to)
        : seg_id(id)
        , turn_index(-1)
    {
        points[0] = from;
        points[1] = to;
    }
};


template <typename Point>
struct angle_info
{
    segment_identifier seg_id;
    Point point;
    bool incoming;
    bool blocked;

    inline angle_info(segment_identifier const& id, bool inc, Point const& p)
        : seg_id(id)
        , point(p)
        , incoming(inc)
        , blocked(false)
    {
    }
};

template <typename Vector>
inline int get_quadrant(Vector const& vector)
{
    // Return quadrant as layouted in the code below:
    // 3 | 0
    // -----
    // 2 | 1
    return geometry::get<1>(vector) >= 0
        ? (geometry::get<0>(vector)  < 0 ? 3 : 0)
        : (geometry::get<0>(vector)  < 0 ? 2 : 1)
        ;
}

template <typename Vector>
inline int squared_length(Vector const& vector)
{
    return geometry::get<0>(vector) * geometry::get<0>(vector)
         + geometry::get<1>(vector) * geometry::get<1>(vector)
         ;
}


template <typename Point>
struct angle_less
{
    typedef Point vector_type;
    typedef typename strategy::side::services::default_strategy
    <
        typename cs_tag<Point>::type
    >::type side_strategy_type;

    angle_less(Point const& origin)
        : m_origin(origin)
    {}

    template <typename Angle>
    inline bool operator()(Angle const& p, Angle const& q) const
    {
        // Vector origin -> p and origin -> q
        vector_type pv = p.point;
        vector_type qv = q.point;
        geometry::subtract_point(pv, m_origin);
        geometry::subtract_point(qv, m_origin);

        int const quadrant_p = get_quadrant(pv);
        int const quadrant_q = get_quadrant(qv);
        if (quadrant_p != quadrant_q)
        {
            return quadrant_p < quadrant_q;
        }
        // Same quadrant, check if p is located left of q
        int const side = side_strategy_type::apply(m_origin, q.point,
                    p.point);
        if (side != 0)
        {
            return side == 1;
        }
        // Collinear, check if one is incoming
        if (p.incoming != q.incoming)
        {
            return int(p.incoming) < int(q.incoming);
        }
        // Same quadrant/side/direction, return longest first
        int const length_p = squared_length(pv);
        int const length_q = squared_length(qv);
        if (length_p != length_q)
        {
            return squared_length(pv) > squared_length(qv);
        }
        // They are still the same. Just compare on seg_id
        return p.seg_id < q.seg_id;
    }

private:
    Point m_origin;
};

template <typename Point>
struct angle_equal_to
{
    typedef Point vector_type;
    typedef typename strategy::side::services::default_strategy
    <
        typename cs_tag<Point>::type
    >::type side_strategy_type;

    inline angle_equal_to(Point const& origin)
        : m_origin(origin)
    {}

    template <typename Angle>
    inline bool operator()(Angle const& p, Angle const& q) const
    {
        // Vector origin -> p and origin -> q
        vector_type pv = p.point;
        vector_type qv = q.point;
        geometry::subtract_point(pv, m_origin);
        geometry::subtract_point(qv, m_origin);

        if (get_quadrant(pv) != get_quadrant(qv))
        {
            return false;
        }
        // Same quadrant, check if p/q are collinear
        int const side = side_strategy_type::apply(m_origin, q.point,
                    p.point);
        return side == 0;
    }

private:
    Point m_origin;
};

struct left_turn
{
    segment_identifier from;
    segment_identifier to;
};


template <typename Point, typename AngleCollection, typename OutputCollection>
inline void get_left_turns(AngleCollection const& sorted_angles, Point const& origin,
        OutputCollection& output_collection)
{
    angle_equal_to<Point> comparator(origin);
    typedef geometry::closing_iterator<AngleCollection const> closing_iterator;
    closing_iterator it(sorted_angles);
    closing_iterator end(sorted_angles, true);

    closing_iterator previous = it++;
    for( ; it != end; previous = it++)
    {
        if (! it->blocked)
        {
            bool equal = comparator(*previous, *it);
            bool include = ! equal
                && previous->incoming
                && !it->incoming;
            if (include)
            {
                left_turn turn;
                turn.from = previous->seg_id;
                turn.to = it->seg_id;
                output_collection.push_back(turn);
            }
        }
    }
}

template <typename AngleTurnCollection, typename AngleCollection>
inline void block_turns_on_right_sides(AngleTurnCollection const& turns,
        AngleCollection& sorted)
{
    // Create a small (seg_id -> index) map for fast finding turns
    std::map<segment_identifier, int> incoming;
    std::map<segment_identifier, int> outgoing;
    int index = 0;
    for (typename boost::range_iterator<AngleCollection>::type it =
        sorted.begin(); it != sorted.end(); ++it, ++index)
    {
        if (it->incoming)
        {
            incoming[it->seg_id] = index;
        }
        else
        {
            outgoing[it->seg_id] = index;
        }
    }

    // Walk through turns and block every outgoing angle on the right side
    for (typename boost::range_iterator<AngleTurnCollection const>::type it =
        turns.begin(); it != turns.end(); ++it)
    {
        int incoming_index = incoming[it->seg_id];
        int outgoing_index = outgoing[it->seg_id];
        int index = incoming_index;
        while(index != outgoing_index)
        {
            if (!sorted[index].incoming)
            {
                sorted[index].blocked = true;
            }

            // Go back (circular)
            index--;
            if (index == -1)
            {
                index = boost::size(sorted) - 1;
            }
        }
    }
}

template <typename AngleCollection, typename Point>
inline bool has_rounding_issues(AngleCollection const& angles, Point const& origin)
{
    for (typename boost::range_iterator<AngleCollection const>::type it =
        angles.begin(); it != angles.end(); ++it)
    {
        // Vector origin -> p and origin -> q
        typedef Point vector_type;
        vector_type v = it->point;
        geometry::subtract_point(v, origin);
        return geometry::math::abs(geometry::get<0>(v)) <= 1
            || geometry::math::abs(geometry::get<1>(v)) <= 1
            ;
    }
    return false;
}


}  // namespace left_turns

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_GET_LEFT_TURNS_HPP
