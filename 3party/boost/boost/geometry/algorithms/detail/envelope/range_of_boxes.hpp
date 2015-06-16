// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_OF_BOXES_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_OF_BOXES_HPP

#include <cstddef>

#include <algorithm>
#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/detail/max_interval_gap.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


template <typename T>
class longitude_interval
{
    typedef T const& reference_type;

public:
    typedef T value_type;
    typedef T difference_type;

    longitude_interval(T const& left, T const& right)
    {
        m_end[0] = left;
        m_end[1] = right;        
    }

    template <std::size_t Index>
    reference_type get() const
    {
        return m_end[Index];
    }

    difference_type length() const
    {
        return get<1>() - get<0>();
    }

private:
    T m_end[2];
};


template <typename Units>
struct envelope_range_of_longitudes
{
    template <std::size_t Index>
    struct longitude_less
    {
        template <typename Interval>
        inline bool operator()(Interval const& i1, Interval const& i2) const
        {
            return math::smaller(i1.template get<Index>(),
                                 i2.template get<Index>());
        }
    };

    template <typename RangeOfLongitudeIntervals, typename Longitude>
    static inline void apply(RangeOfLongitudeIntervals const& range,
                             Longitude& lon_min, Longitude& lon_max)
    {
        typedef typename math::detail::constants_on_spheroid
            <
                Longitude, Units
            > constants;

        Longitude const zero = 0;
        Longitude const period = constants::period();
        Longitude const min_longitude = constants::min_longitude();
        Longitude const max_longitude = constants::max_longitude();

        lon_min = lon_max = zero;

        // the range of longitude intervals can be empty if all input boxes
        // degenerate to the north or south pole (or combination of the two)
        // in this case the initialization values for lon_min and
        // lon_max are valid choices
        if (! boost::empty(range))
        {
            lon_min = std::min_element(boost::begin(range),
                                       boost::end(range),
                                       longitude_less<0>())->template get<0>();
            lon_max = std::max_element(boost::begin(range),
                                       boost::end(range),
                                       longitude_less<1>())->template get<1>();

            if (math::larger(lon_max - lon_min, constants::half_period()))
            {
                Longitude max_gap_left, max_gap_right;
                Longitude max_gap = geometry::maximum_gap(range,
                                                          max_gap_left,
                                                          max_gap_right);

                BOOST_GEOMETRY_ASSERT(! math::larger(lon_min, lon_max));
                BOOST_GEOMETRY_ASSERT(! math::larger(lon_max, max_longitude));
                BOOST_GEOMETRY_ASSERT(! math::smaller(lon_min, min_longitude));

                BOOST_GEOMETRY_ASSERT(! math::larger(max_gap_left, max_gap_right));
                BOOST_GEOMETRY_ASSERT(! math::larger(max_gap_right, max_longitude));
                BOOST_GEOMETRY_ASSERT(! math::smaller(max_gap_left, min_longitude));

                if (math::larger(max_gap, zero))
                {
                    Longitude wrapped_gap = period + lon_min - lon_max;
                    if (math::larger(max_gap, wrapped_gap))
                    {
                        lon_min = max_gap_right;
                        lon_max = max_gap_left + period;
                    }
                }
            }
        }
    }
};


struct envelope_range_of_boxes
{
    template <std::size_t Index>
    struct latitude_less
    {
        template <typename Box>
        inline bool operator()(Box const& box1, Box const& box2) const
        {
            return math::smaller(geometry::get<Index, 1>(box1),
                                 geometry::get<Index, 1>(box2));
        }
    };

    template <typename RangeOfBoxes, typename Box>
    static inline void apply(RangeOfBoxes const& range_of_boxes, Box& mbr)
    {
        // boxes in the range are assumed to be normalized already

        typedef typename boost::range_value<RangeOfBoxes>::type box_type;
        typedef typename coordinate_type<box_type>::type coordinate_type;
        typedef typename coordinate_system<box_type>::type::units units_type;
        typedef typename boost::range_iterator
            <
                RangeOfBoxes const
            >::type iterator_type;

        typedef math::detail::constants_on_spheroid
            <
                coordinate_type, units_type
            > constants;

        typedef longitude_interval<coordinate_type> interval_type;
        typedef std::vector<interval_type> interval_range_type;

        BOOST_GEOMETRY_ASSERT(! boost::empty(range_of_boxes));

        iterator_type it_min = std::min_element(boost::begin(range_of_boxes),
                                                boost::end(range_of_boxes),
                                                latitude_less<min_corner>());
        iterator_type it_max = std::max_element(boost::begin(range_of_boxes),
                                                boost::end(range_of_boxes),
                                                latitude_less<max_corner>());

        coordinate_type const min_longitude = constants::min_longitude();
        coordinate_type const max_longitude = constants::max_longitude();
        coordinate_type const period = constants::period();

        interval_range_type intervals;
        for (iterator_type it = boost::begin(range_of_boxes);
             it != boost::end(range_of_boxes);
             ++it)
        {
            coordinate_type lat_min = geometry::get<min_corner, 1>(*it);
            coordinate_type lat_max = geometry::get<max_corner, 1>(*it);
            if (math::equals(lat_min, constants::max_latitude())
                || math::equals(lat_max, constants::min_latitude()))
            {
                // if the box degenerates to the south or north pole
                // just ignore it
                continue;
            }                             

            coordinate_type lon_left = geometry::get<min_corner, 0>(*it);
            coordinate_type lon_right = geometry::get<max_corner, 0>(*it);

            if (math::larger(lon_right, max_longitude))
            {
                intervals.push_back(interval_type(lon_left, max_longitude));
                intervals.push_back
                    (interval_type(min_longitude, lon_right - period));
            }
            else
            {
                intervals.push_back(interval_type(lon_left, lon_right));
            }
        }

        coordinate_type lon_min(0), lon_max(0);
        envelope_range_of_longitudes
            <
                units_type
            >::apply(intervals, lon_min, lon_max);

        // do not convert units; conversion will be performed at a
        // higher level
        assign_values(mbr,
                      lon_min,
                      geometry::get<min_corner, 1>(*it_min),
                      lon_max,
                      geometry::get<max_corner, 1>(*it_max));
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_RANGE_OF_BOXES_HPP
