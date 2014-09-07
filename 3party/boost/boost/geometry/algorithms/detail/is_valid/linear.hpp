// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP

#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/is_valid/has_spikes.hpp>
#include <boost/geometry/algorithms/detail/num_distinct_consecutive_points.hpp>

#include <boost/geometry/algorithms/dispatch/is_valid.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace is_valid
{


template <typename Linestring, bool AllowSpikes>
struct is_valid_linestring
{
    static inline bool apply(Linestring const& linestring)
    {
        std::size_t num_distinct = detail::num_distinct_consecutive_points
            <
                Linestring,
                3u,
                true,
                not_equal_to<typename point_type<Linestring>::type>
            >::apply(linestring);

        if ( num_distinct < 2u )
        {
            return false;
        }

        return num_distinct == 2u
            || AllowSpikes
            || !has_spikes<Linestring, closed>::apply(linestring);
    }
};


}} // namespace detail::is_valid
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// A linestring is a curve.
// A curve is 1-dimensional so it has to have at least two distinct
// points.
// A curve is simple if it does not pass through the same point twice,
// with the possible exception of its two endpoints
//
// There is an option here as to whether spikes are allowed for linestrings; 
// here we pass this as an additional template parameter: allow_spikes
// If allow_spikes is set to true, spikes are allowed, false otherwise.
// By default, spikes are disallowed
//
// Reference: OGC 06-103r4 (6.1.6.1)
template <typename Linestring, bool AllowSpikes>
struct is_valid<Linestring, linestring_tag, AllowSpikes>
    : detail::is_valid::is_valid_linestring<Linestring, AllowSpikes>
{};


// A MultiLinestring is a MultiCurve
// A MultiCurve is simple if all of its elements are simple and the
// only intersections between any two elements occur at Points that
// are on the boundaries of both elements.
//
// Reference: OGC 06-103r4 (6.1.8.1; Fig. 9)
template <typename MultiLinestring, bool AllowSpikes>
struct is_valid<MultiLinestring, multi_linestring_tag, AllowSpikes>
{
    static inline bool apply(MultiLinestring const& multilinestring)
    {
        return detail::check_iterator_range
            <
                detail::is_valid::is_valid_linestring
                    <
                        typename boost::range_value<MultiLinestring>::type,
                        AllowSpikes
                    >,
                false // do not allow empty multilinestring
            >::apply(boost::begin(multilinestring),
                     boost::end(multilinestring));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_LINEAR_HPP
