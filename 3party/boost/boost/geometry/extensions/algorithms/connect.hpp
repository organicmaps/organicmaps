// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_CONNECT_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_CONNECT_HPP

#include <map>


#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/strategies/default_distance_result.hpp>
#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/io/dsv/write.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace connect
{


template <typename Point>
struct node
{
    int index;
    bool is_from;
    Point point;

    node(int i, bool f, Point const& p)
        : index(i)
        , is_from(f)
        , point(p)
    {}

    node()
        : index(-1)
        , is_from(false)
    {}
};

template <typename Point>
struct map_policy
{
    typedef typename strategy::distance::services::default_strategy
        <
            point_tag, point_tag, Point
        >::type strategy_type;

    // Have a map<point, <index,start/end> > such that we can find
    // the corresponding point on each end. Note that it uses the
    // default "equals" for the point-type
    typedef std::map
        <
            Point,
            std::vector<node<Point> >,
            geometry::less<Point>
        > map_type;

    typedef typename map_type::const_iterator map_iterator_type;
    typedef typename std::vector<node<Point> >::const_iterator vector_iterator_type;

    typedef Point point_type;
    typedef typename default_distance_result<Point>::type distance_result_type;


    map_type map;


    inline bool find_start(node<Point>& object,
            std::map<int, bool>& included,
            int expected_count = 1)
    {
        for (map_iterator_type it = map.begin();
            it != map.end();
            ++it)
        {
            if ((expected_count == 1 && boost::size(it->second) == 1)
                || (expected_count > 1 && boost::size(it->second) > 1))
            {
                for (vector_iterator_type vit = it->second.begin();
                    vit != it->second.end();
                    ++vit)
                {
                    if (! included[vit->index])
                    {
                        included[vit->index] = true;
                        object = *vit;
                        return true;
                    }
                }
            }
        }

        // Not found with one point, try one with two points
        // to find rings
        if (expected_count == 1)
        {
            return find_start(object, included, 2);
        }

        return false;
    }

    inline void add(int index, Point const& p, bool is_from)
    {
        map[p].push_back(node<Point>(index, is_from, p));
    }


    template <typename LineString>
    inline void add(int index, LineString const& ls)
    {
        if (boost::size(ls) > 0)
        {
            add(index, *boost::begin(ls), true);
            add(index, *(boost::end(ls) - 1), false);
        }
    }

    inline node<Point> find_closest(Point const& p1, std::map<int, bool>& included)
    {
        std::vector<node<Point> > const& range = map[p1];

        node<Point> closest;


        // Alternatively, we might look for the closest points
        if (boost::size(range) == 0)
        {
            std::cout << "nothing found" << std::endl;
            return closest;
        }

        // 2c: for all candidates get closest one
        strategy_type strategy;

        distance_result_type min_dist = strategy::distance::services
            ::result_from_distance<strategy_type, Point, Point>::apply(strategy, 100);

        for (vector_iterator_type it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            if (! included[it->index])
            {
                distance_result_type d = geometry::comparable_distance(p1, it->point);
                if (d < min_dist)
                {
                    closest = *it;
                    min_dist = d;

                    //std::cout << "TO " << geometry::wkt(p2) << std::endl;
                }
            }
        }
        return closest;
    }

};


template <typename Point>
struct fuzzy_policy
{
    typedef typename strategy::distance::services::default_strategy
        <
            point_tag, point_tag, Point
        >::type strategy_type;

    // Have a map<point, <index,start/end> > such that we can find
    // the corresponding point on each end. Note that it uses the
    // default "equals" for the point-type
    typedef std::vector
        <
            std::pair
                <
                    Point,
                    std::vector<node<Point> >
                >
        > map_type;

    typedef typename map_type::const_iterator map_iterator_type;
    typedef typename std::vector<node<Point> >::const_iterator vector_iterator_type;

    typedef Point point_type;
    typedef typename default_distance_result<Point>::type distance_result_type;


    map_type map;
    typename coordinate_type<Point>::type m_limit;


    fuzzy_policy(typename coordinate_type<Point>::type limit)
        : m_limit(limit)
    {}

    inline bool find_start(node<Point>& object,
            std::map<int, bool>& included,
            int expected_count = 1)
    {
        for (map_iterator_type it = map.begin();
            it != map.end();
            ++it)
        {
            if ((expected_count == 1 && boost::size(it->second) == 1)
                || (expected_count > 1 && boost::size(it->second) > 1))
            {
                for (vector_iterator_type vit = it->second.begin();
                    vit != it->second.end();
                    ++vit)
                {
                    if (! included[vit->index])
                    {
                        included[vit->index] = true;
                        object = *vit;
                        return true;
                    }
                }
            }
        }

        // Not found with one point, try one with two points
        // to find rings
        if (expected_count == 1)
        {
            return find_start(object, included, 2);
        }

        return false;
    }

    inline typename boost::range_iterator<map_type>::type fuzzy_closest(Point const& p)
    {
        typename boost::range_iterator<map_type>::type closest = boost::end(map);

        for (typename boost::range_iterator<map_type>::type it = boost::begin(map);
            it != boost::end(map);
            ++it)
        {
            distance_result_type d = geometry::distance(p, it->first);
            if (d < m_limit)
            {
                if (closest == boost::end(map))
                {
                    closest = it;
                }
                else
                {
                    distance_result_type dc = geometry::distance(p, closest->first);
                    if (d < dc)
                    {
                        closest = it;
                    }
                }
            }
        }
        return closest;
    }


    inline void add(int index, Point const& p, bool is_from)
    {
        // Iterate through all points and get the closest one.
        typename boost::range_iterator<map_type>::type it = fuzzy_closest(p);
        if (it == map.end())
        {
            map.resize(map.size() + 1);
            map.back().first = p;
            it = map.end() - 1;
        }
        it->second.push_back(node<Point>(index, is_from, p));
    }


    template <typename LineString>
    inline void add(int index, LineString const& ls)
    {
        if (boost::size(ls) > 0)
        {
            add(index, *boost::begin(ls), true);
            add(index, *(boost::end(ls) - 1), false);
        }
    }

    inline node<Point> find_closest(Point const& p1, std::map<int, bool>& included)
    {
        namespace services = strategy::distance::services;

        node<Point> closest;

        typename boost::range_iterator<map_type>::type it = fuzzy_closest(p1);
        if (it == map.end())
        {
            return closest;
        }

        std::vector<node<Point> > const& range = it->second;



        // Alternatively, we might look for the closest points
        if (boost::size(range) == 0)
        {
            std::cout << "nothing found" << std::endl;
            return closest;
        }

        // 2c: for all candidates get closest one
        strategy_type strategy;
        distance_result_type min_dist = strategy::distance::services
            ::result_from_distance<strategy_type, Point, Point>::apply(strategy, 100);

        for (vector_iterator_type it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            if (! included[it->index])
            {
                distance_result_type d = geometry::comparable_distance(p1, it->point);
                if (d < min_dist)
                {
                    closest = *it;
                    min_dist = d;

                    //std::cout << "TO " << geometry::wkt(p2) << std::endl;
                }
            }
        }
        return closest;
    }
};

template <typename Policy>
inline void debug(Policy const& policy)
{
    std::cout << "MAP" << std::endl;
    typedef typename Policy::map_type::const_iterator iterator;
    typedef typename Policy::point_type point_type;

    for (iterator it=policy.map.begin(); it != policy.map.end(); ++it)
    {
        std::cout << geometry::dsv(it->first) << " => " ;
        std::vector<node<point_type> > const& range =it->second;
        for (typename std::vector<node<point_type> >::const_iterator
            vit = boost::begin(range); vit != boost::end(range); ++vit)
        {
            std::cout
                << " (" << vit->index
                << ", " << (vit->is_from ? "F" : "T")
                << ")"
                ;
        }
        std::cout << std::endl;
    }
}




// Dissolve on multi_linestring tries to create larger linestrings from input,
// or closed rings.

template <typename Multi, typename GeometryOut, typename Policy>
struct connect_multi_linestring
{
    typedef typename point_type<Multi>::type point_type;
    typedef typename boost::range_iterator<Multi const>::type iterator_type;
    typedef typename boost::range_value<Multi>::type linestring_type;


    static inline void copy(linestring_type const& ls,
            GeometryOut& target,
            bool copy_forward)
    {
        if (copy_forward)
        {
            std::copy(boost::begin(ls), boost::end(ls),
                std::back_inserter(target));
        }
        else
        {
            std::reverse_copy(boost::begin(ls), boost::end(ls),
                std::back_inserter(target));
        }
    }


    template <typename OutputIterator>
    static inline OutputIterator apply(Multi const& multi, Policy& policy, OutputIterator out)
    {
        if (boost::size(multi) <= 0)
        {
            return out;
        }

        // 1: fill the map.
        int index = 0;
        for (iterator_type it = boost::begin(multi);
            it != boost::end(multi);
            ++it, ++index)
        {
            policy.add(index, *it);
        }

        debug(policy);

        std::map<int, bool> included;

        // 2: connect the lines

        // 2a: start with one having a unique starting point
        node<point_type> starting_point;
        if (! policy.find_start(starting_point, included))
        {
            return out;
        }

        GeometryOut current;
        copy(multi[starting_point.index], current, starting_point.is_from);

        bool found = true;
        while(found)
        {
            // 2b: get all candidates, by asking multi-map for range
            point_type const& p1 = *(boost::end(current) - 1);

            node<point_type> closest = policy.find_closest(p1, included);

            found = false;

            // 2d: if there is a closest one add it
            if (closest.index >= 0)
            {
                found = true;
                included[closest.index] = true;
                copy(multi[closest.index], current, closest.is_from);
            }
            else if ((included.size() != std::size_t(boost::size(multi))))
            {
                // Get one which is NOT found and go again
                node<point_type> next;
                if (policy.find_start(next, included))
                {
                    found = true;

                    *out++ = current;
                    geometry::clear(current);

                    copy(multi[next.index], current, next.is_from);
                }
            }
        }
        if (boost::size(current) > 0)
        {
            *out++ = current;
        }

        return out;
    }
};

}} // namespace detail::connect
#endif



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag,
    typename GeometryOutTag,
    typename Geometry,
    typename GeometryOut,
    typename Policy
>
struct connect
{};


template<typename Multi, typename GeometryOut, typename Policy>
struct connect<multi_linestring_tag, linestring_tag, Multi, GeometryOut, Policy>
    : detail::connect::connect_multi_linestring
        <
            Multi,
            GeometryOut,
            Policy
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template
<
    typename Geometry,
    typename Collection
>
inline void connect(Geometry const& geometry, Collection& output_collection)
{
    typedef typename boost::range_value<Collection>::type geometry_out;

    concept::check<Geometry const>();
    concept::check<geometry_out>();

    typedef detail::connect::map_policy
        <
            typename point_type<Geometry>::type
        > policy_type;

    policy_type policy;

    dispatch::connect
    <
        typename tag<Geometry>::type,
        typename tag<geometry_out>::type,
        Geometry,
        geometry_out,
        policy_type
    >::apply(geometry, policy, std::back_inserter(output_collection));
}



template
<
    typename Geometry,
    typename Collection
>
inline void connect(Geometry const& geometry, Collection& output_collection,
            typename coordinate_type<Geometry>::type const& limit)
{
    typedef typename boost::range_value<Collection>::type geometry_out;

    concept::check<Geometry const>();
    concept::check<geometry_out>();

    typedef detail::connect::fuzzy_policy
        <
            typename point_type<Geometry>::type
        > policy_type;

    policy_type policy(limit);

    dispatch::connect
    <
        typename tag<Geometry>::type,
        typename tag<geometry_out>::type,
        Geometry,
        geometry_out,
        policy_type
    >::apply(geometry, policy, std::back_inserter(output_collection));
}



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_CONNECT_HPP
