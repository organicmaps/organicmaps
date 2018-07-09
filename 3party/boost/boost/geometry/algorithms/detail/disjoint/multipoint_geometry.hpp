// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2017, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_MULTIPOINT_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_MULTIPOINT_GEOMETRY_HPP

#include <algorithm>
#include <vector>

#include <boost/range.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/iterators/segment_iterator.hpp>

#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/disjoint/multirange_geometry.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_point.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_geometry.hpp>
#include <boost/geometry/algorithms/detail/relate/less.hpp>

#include <boost/geometry/algorithms/dispatch/disjoint.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


template <typename MultiPoint1, typename MultiPoint2>
class multipoint_multipoint
{
private:
    template <typename Iterator>
    class unary_disjoint_predicate
        : detail::relate::less
    {
    private:
        typedef detail::relate::less base_type;

    public:
        unary_disjoint_predicate(Iterator first, Iterator last)
            : base_type(), m_first(first), m_last(last)
        {}

        template <typename Point>
        inline bool apply(Point const& point) const
        {
            return !std::binary_search(m_first,
                                       m_last,
                                       point,
                                       static_cast<base_type const&>(*this));
        }

    private:
        Iterator m_first, m_last;
    };

public:
    static inline bool apply(MultiPoint1 const& multipoint1,
                             MultiPoint2 const& multipoint2)
    {
        BOOST_GEOMETRY_ASSERT( boost::size(multipoint1) <= boost::size(multipoint2) );

        typedef typename boost::range_value<MultiPoint1>::type point1_type;

        std::vector<point1_type> points1(boost::begin(multipoint1),
                                         boost::end(multipoint1));

        std::sort(points1.begin(), points1.end(), detail::relate::less());

        typedef unary_disjoint_predicate
            <
                typename std::vector<point1_type>::const_iterator
            > predicate_type;

        return check_iterator_range
            <
                predicate_type
            >::apply(boost::begin(multipoint2),
                     boost::end(multipoint2),
                     predicate_type(points1.begin(), points1.end()));
    }
};


template <typename MultiPoint, typename Linear>
class multipoint_linear
{
private:
    struct expand_box_point
    {
        template <typename Box, typename Point>
        static inline void apply(Box& total, Point const& point)
        {
            geometry::expand(total, point);
        }
    };

    // TODO: After adding non-cartesian Segment envelope to the library
    // this policy should be modified to take envelope strategy.
    struct expand_box_segment
    {
        template <typename Box, typename Segment>
        static inline void apply(Box& total, Segment const& segment)
        {
            geometry::expand(total, geometry::return_envelope<Box>(segment));
        }
    };

    struct overlaps_box_point
    {
        template <typename Box, typename Point>
        static inline bool apply(Box const& box, Point const& point)
        {
            // The default strategy is enough in this case
            typedef typename strategy::disjoint::services::default_strategy
                <
                    Point, Box
                >::type strategy_type;
            return ! dispatch::disjoint<Point, Box>::apply(point, box, strategy_type());
        }
    };

    // TODO: After implementing disjoint Segment/Box for non-cartesian geometries
    // this strategy should be passed here.
    // TODO: This Segment/Box strategy should somehow be derived from Point/Segment strategy
    // which by default is winding containing CS-specific side strategy
    // TODO: disjoint Segment/Box will be called in this case which may take
    // quite long in non-cartesian CS. So we should consider passing range of bounding boxes
    // of segments after calculating them once.
    struct overlaps_box_segment
    {
        template <typename Box, typename Segment>
        static inline bool apply(Box const& box, Segment const& segment)
        {
            typedef typename strategy::disjoint::services::default_strategy
                <
                    Segment, Box
                >::type strategy_type;
            return ! dispatch::disjoint<Segment, Box>::apply(segment, box, strategy_type());
        }
    };

    template <typename PtSegStrategy>
    class item_visitor_type
    {
    public:
        item_visitor_type(PtSegStrategy const& strategy)
            : m_intersection_found(false)
            , m_strategy(strategy)
        {}

        template <typename Item1, typename Item2>
        inline void apply(Item1 const& item1, Item2 const& item2)
        {
            if (! m_intersection_found
                && ! dispatch::disjoint<Item1, Item2>::apply(item1, item2, m_strategy))
            {
                m_intersection_found = true;
            }
        }

        inline bool intersection_found() const { return m_intersection_found; }

    private:
        bool m_intersection_found;
        PtSegStrategy const& m_strategy;
    };
    // structs for partition -- end

    class segment_range
    {
    public:
        typedef geometry::segment_iterator<Linear const> const_iterator;
        typedef const_iterator iterator;

        segment_range(Linear const& linear)
            : m_linear(linear)
        {}

        const_iterator begin() const
        {
            return geometry::segments_begin(m_linear);
        }

        const_iterator end() const
        {
            return geometry::segments_end(m_linear);
        }

    private:
        Linear const& m_linear;
    };

public:
    template <typename Strategy>
    static inline bool apply(MultiPoint const& multipoint, Linear const& linear, Strategy const& strategy)
    {
        item_visitor_type<Strategy> visitor(strategy);

        geometry::partition
            <
                geometry::model::box<typename point_type<MultiPoint>::type>
            >::apply(multipoint, segment_range(linear), visitor,
                     expand_box_point(), overlaps_box_point(),
                     expand_box_segment(), overlaps_box_segment());

        return ! visitor.intersection_found();
    }

    template <typename Strategy>
    static inline bool apply(Linear const& linear, MultiPoint const& multipoint, Strategy const& strategy)
    {
        return apply(multipoint, linear, strategy);
    }
};


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Point, typename MultiPoint, std::size_t DimensionCount>
struct disjoint
    <
        Point, MultiPoint, DimensionCount, point_tag, multi_point_tag, false
    > : detail::disjoint::multirange_constant_size_geometry<MultiPoint, Point>
{};


template <typename MultiPoint, typename Segment, std::size_t DimensionCount>
struct disjoint
    <
        MultiPoint, Segment, DimensionCount, multi_point_tag, segment_tag, false
    > : detail::disjoint::multirange_constant_size_geometry<MultiPoint, Segment>
{};


template <typename MultiPoint, typename Box, std::size_t DimensionCount>
struct disjoint
    <
        MultiPoint, Box, DimensionCount, multi_point_tag, box_tag, false
    > : detail::disjoint::multirange_constant_size_geometry<MultiPoint, Box>
{};


template
<
    typename MultiPoint1,
    typename MultiPoint2,
    std::size_t DimensionCount
>
struct disjoint
    <
        MultiPoint1, MultiPoint2, DimensionCount,
        multi_point_tag, multi_point_tag, false
    >
{
    template <typename Strategy>
    static inline bool apply(MultiPoint1 const& multipoint1,
                             MultiPoint2 const& multipoint2,
                             Strategy const& )
    {
        if ( boost::size(multipoint2) < boost::size(multipoint1) )
        {
            return detail::disjoint::multipoint_multipoint
                <
                    MultiPoint2, MultiPoint1
                >::apply(multipoint2, multipoint1);
        } 

        return detail::disjoint::multipoint_multipoint
            <
                MultiPoint1, MultiPoint2
            >::apply(multipoint1, multipoint2);
   }
};


template <typename Linear, typename MultiPoint, std::size_t DimensionCount>
struct disjoint
    <
        Linear, MultiPoint, DimensionCount, linear_tag, multi_point_tag, false
    > : detail::disjoint::multipoint_linear<MultiPoint, Linear>
{};


template <typename MultiPoint, typename Linear, std::size_t DimensionCount>
struct disjoint
    <
        MultiPoint, Linear, DimensionCount, multi_point_tag, linear_tag, false
    > : detail::disjoint::multipoint_linear<MultiPoint, Linear>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry



#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_MULTIPOINT_GEOMETRY_HPP
