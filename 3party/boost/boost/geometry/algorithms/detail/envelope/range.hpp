// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP

#include <boost/range.hpp>
#include <boost/geometry/util/range.hpp>

#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/algorithms/dispatch/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


struct default_expand_policy
{
    template <typename Box, typename Geometry>
    static inline void apply(Box& mbr, Geometry const& geometry)
    {
        geometry::expand(mbr, geometry);
    }
};


template <typename ExpandPolicy, typename Iterator, typename Box>
inline void envelope_iterators(Iterator first, Iterator last, Box& mbr)
{
    for (Iterator it = first; it != last; ++it)
    {
        ExpandPolicy::apply(mbr, *it);
    }
}


template <typename ExpandPolicy = default_expand_policy>
struct envelope_range
{
    /// Calculate envelope of range using a strategy
    template <typename Range, typename Box>
    static inline void apply(Range const& range, Box& mbr)
    {
        typename boost::range_iterator<Range const>::type it 
            = boost::begin(range);

        if (it != boost::end(range))
        {
            // initialize box with first element in range
            dispatch::envelope
                <
                    typename boost::range_value<Range>::type
                >::apply(*it, mbr);

            // consider now the remaining elements in the range (if any)
            ++it;
            envelope_iterators
                <
                    ExpandPolicy
                >(it, boost::end(range), mbr);
        }
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_HPP
