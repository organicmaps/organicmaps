// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP

#include <algorithm>
#include <deque>

#include <boost/assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/policies/predicate_based_interrupt_policy.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>
#include <boost/geometry/policies/robustness/segment_ratio.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/intersects.hpp>

#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>

#include <boost/geometry/algorithms/detail/disjoint/linear_linear.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_duplicates.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_spikes.hpp>

#include <boost/geometry/algorithms/detail/is_simple/debug_print_boundary_points.hpp>
#include <boost/geometry/algorithms/detail/is_valid/debug_print_turns.hpp>

#include <boost/geometry/algorithms/dispatch/is_simple.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_simple
{


template <typename Linestring, bool CheckSelfIntersections = true>
struct is_simple_linestring
{
    static inline bool apply(Linestring const& linestring)
    {
        return !detail::is_valid::has_duplicates
                    <
                        Linestring, closed
                    >::apply(linestring)
            && !detail::is_valid::has_spikes
                    <
                        Linestring, closed
                    >::apply(linestring)
            && !(CheckSelfIntersections && geometry::intersects(linestring));
    }
};



template <typename MultiLinestring>
class is_simple_multilinestring
{
private:
    class is_acceptable_turn
    {
    private:
        template <typename Point, typename Linestring>
        static inline bool is_boundary_point_of(Point const& point,
                                                Linestring const& linestring)
        {
            BOOST_ASSERT( boost::size(linestring) > 1 );
            return
                !geometry::equals(range::front(linestring),
                                  range::back(linestring))
                &&
                ( geometry::equals(point, range::front(linestring))
                  || geometry::equals(point, range::back(linestring)) );
        }

        template <typename Linestring1, typename Linestring2>
        static inline bool have_same_boundary_points(Linestring1 const& ls1,
                                                     Linestring2 const& ls2)
        {
            return
                geometry::equals(range::front(ls1), range::front(ls2))
                ?
                geometry::equals(range::back(ls1), range::back(ls2))
                :
                (geometry::equals(range::front(ls1), range::back(ls2))
                 &&
                 geometry::equals(range::back(ls1), range::front(ls2))
                 )
                ;
        }

    public:
        is_acceptable_turn(MultiLinestring const& multilinestring)
            : m_multilinestring(multilinestring)
        {}

        template <typename Turn>
        inline bool apply(Turn const& turn) const
        {
            typedef typename boost::range_value
                <
                    MultiLinestring
                >::type linestring;

            linestring const& ls1 =
                range::at(m_multilinestring,
                          turn.operations[0].seg_id.multi_index);

            linestring const& ls2 =
                range::at(m_multilinestring,
                          turn.operations[0].other_id.multi_index);

            return
                is_boundary_point_of(turn.point, ls1)
                && is_boundary_point_of(turn.point, ls2)
                &&
                ( boost::size(ls1) != 2
                  || boost::size(ls2) != 2
                  || !have_same_boundary_points(ls1, ls2) );
        }

    private:
        MultiLinestring const& m_multilinestring;        
    };


public:
    static inline bool apply(MultiLinestring const& multilinestring)
    {
        typedef typename boost::range_value<MultiLinestring>::type linestring;
        typedef typename point_type<MultiLinestring>::type point_type;
        typedef point_type point;


        // check each of the linestrings for simplicity
        if ( !detail::check_iterator_range
                 <
                     is_simple_linestring<linestring>,
                     false // do not allow empty multilinestring
                 >::apply(boost::begin(multilinestring),
                          boost::end(multilinestring))
             )
        {
            return false;
        }


        // compute self turns
        typedef detail::overlay::turn_info
            <
                point_type,
                geometry::segment_ratio
                <
                    typename geometry::coordinate_type<point>::type
                >
            > turn_info;

        std::deque<turn_info> turns;

        typedef detail::overlay::get_turn_info
            <
                detail::disjoint::assign_disjoint_policy
            > turn_policy;

        is_acceptable_turn predicate(multilinestring);
        detail::overlay::predicate_based_interrupt_policy
            <
                is_acceptable_turn
            > interrupt_policy(predicate);

        detail::self_get_turn_points::get_turns
            <
                turn_policy
            >::apply(multilinestring,
                     detail::no_rescale_policy(),
                     turns,
                     interrupt_policy);

        detail::is_valid::debug_print_turns(turns.begin(), turns.end());
        debug_print_boundary_points(multilinestring);

        return !interrupt_policy.has_intersections;
    }

};



}} // namespace detail::is_simple
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// A linestring is a curve.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Linestring>
struct is_simple<Linestring, linestring_tag>
    : detail::is_simple::is_simple_linestring<Linestring>
{};


// A MultiLinestring is a MultiCurve
// A MultiCurve is simple if all of its elements are simple and the
// only intersections between any two elements occur at Points that
// are on the boundaries of both elements.
//
// Reference: OGC 06-103r4 (6.1.8.1; Fig. 9)
template <typename MultiLinestring>
struct is_simple<MultiLinestring, multi_linestring_tag>
    : detail::is_simple::is_simple_multilinestring<MultiLinestring>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_SIMPLE_LINEAR_HPP
