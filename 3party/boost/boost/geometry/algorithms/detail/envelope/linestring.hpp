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

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_LINESTRING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_LINESTRING_HPP

#include <iterator>
#include <vector>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/iterators/segment_iterator.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/is_empty.hpp>

#include <boost/geometry/algorithms/detail/envelope/range.hpp>
#include <boost/geometry/algorithms/detail/envelope/range_of_boxes.hpp>

#include <boost/geometry/algorithms/dispatch/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


struct envelope_linear_on_spheroid
{
    template <typename Units, typename Longitude, typename OutputIterator>
    static inline OutputIterator push_interval(Longitude const& lon1,
                                               Longitude const& lon2,
                                               OutputIterator oit)
    {
        typedef longitude_interval<Longitude> interval_type;

        typedef math::detail::constants_on_spheroid
            <
                Longitude, Units
            > constants;

        BOOST_GEOMETRY_ASSERT(! math::larger(lon1, lon2));
        BOOST_GEOMETRY_ASSERT(! math::larger(lon1, constants::max_longitude()));

        if (math::larger(lon2, constants::max_longitude()))
        {
            *oit++ = interval_type(lon1, constants::max_longitude());
            *oit++ = interval_type(constants::min_longitude(),
                                   lon2 - constants::period());
        }
        else
        {
            *oit++ = interval_type(lon1, lon2);
        }
        return oit;
    }

    template <typename Linear, typename Box>
    static inline void apply(Linear const& linear, Box& mbr)
    {
        typedef typename coordinate_type<Box>::type box_coordinate_type;
        typedef longitude_interval<box_coordinate_type> interval_type;

        typedef typename geometry::segment_iterator
            <
                Linear const
            > iterator_type;

        BOOST_GEOMETRY_ASSERT(! geometry::is_empty(linear));

        std::vector<interval_type> longitude_intervals;
        std::back_insert_iterator
            <
                std::vector<interval_type>
            > oit(longitude_intervals);

        box_coordinate_type lat_min = 0, lat_max = 0;
        bool first = true;
        for (iterator_type seg_it = geometry::segments_begin(linear);
             seg_it != geometry::segments_end(linear);
             ++seg_it, first = false)
        {
            Box segment_mbr;
            dispatch::envelope
                <
                    typename std::iterator_traits<iterator_type>::value_type
                >::apply(*seg_it, segment_mbr);

            oit = push_interval
                <
                    typename coordinate_system<Box>::type::units
                >(geometry::get<min_corner, 0>(segment_mbr),
                  geometry::get<max_corner, 0>(segment_mbr),
                  oit);

            if (first)
            {
                lat_min = geometry::get<min_corner, 1>(segment_mbr);
                lat_max = geometry::get<max_corner, 1>(segment_mbr);
            }

            // update min and max latitude, if needed
            if (math::smaller(geometry::get<min_corner, 1>(segment_mbr),
                              lat_min))
            {
                lat_min = geometry::get<min_corner, 1>(segment_mbr);
            }

            if (math::larger(geometry::get<max_corner, 1>(segment_mbr),
                             lat_max))
            {
                lat_max = geometry::get<max_corner, 1>(segment_mbr);
            }
        }

        box_coordinate_type lon_min = 0, lon_max = 0;
        envelope_range_of_longitudes
            <
                typename coordinate_system<Box>::type::units
            >::apply(longitude_intervals, lon_min, lon_max);

        assign_values(mbr, lon_min, lat_min, lon_max, lat_max);
    }
};


template <typename CSTag>
struct envelope_linestring
    : envelope_range<>
{};

template <>
struct envelope_linestring<spherical_equatorial_tag>
    : envelope_linear_on_spheroid
{};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Linestring>
struct envelope<Linestring, linestring_tag>
    : detail::envelope::envelope_linestring<typename cs_tag<Linestring>::type>
{};

} // namespace dispatch
#endif


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_LINESTRING_HPP
