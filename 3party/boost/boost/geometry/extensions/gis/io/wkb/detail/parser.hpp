// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKB_DETAIL_PARSER_HPP
#define BOOST_GEOMETRY_IO_WKB_DETAIL_PARSER_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>

#include <boost/concept_check.hpp>
#include <boost/cstdint.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/extensions/gis/io/wkb/detail/endian.hpp>
#include <boost/geometry/extensions/gis/io/wkb/detail/ogc.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkb
{

template <typename T>
struct value_parser
{
    typedef T value_type;

    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, T& value, byte_order_type::enum_t order)
    {
        // Very basic pre-conditions check on stream of bytes passed in
        BOOST_STATIC_ASSERT((
            boost::is_integral<typename std::iterator_traits<Iterator>::value_type>::value
        ));
        BOOST_STATIC_ASSERT((sizeof(boost::uint8_t) ==
            sizeof(typename std::iterator_traits<Iterator>::value_type)
        ));

        typedef typename std::iterator_traits<Iterator>::difference_type diff_type;
        diff_type const required_size = sizeof(T);
        if (it != end && std::distance(it, end) >= required_size)
        {
            typedef endian::endian_value<T> parsed_value_type;
            parsed_value_type parsed_value;

            // Decide on direcion of endianness translation, detault to native
            if (byte_order_type::xdr == order)
            {
                parsed_value.template load<endian::big_endian_tag>(it);
            }
            else if (byte_order_type::ndr == order)
            {
                parsed_value.template load<endian::little_endian_tag>(it);
            }
            else
            {
                parsed_value.template load<endian::native_endian_tag>(it);
            }

            value = parsed_value;
            std::advance(it, required_size);
            return true;
        }

        return false;
    }
};

struct byte_order_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, byte_order_type::enum_t& order)
    {
        boost::uint8_t value;
        if (value_parser<boost::uint8_t>::parse(it, end, value, byte_order_type::unknown))
        {
            if (byte_order_type::unknown > value)
            {
                order = byte_order_type::enum_t(value);
            }
            return true;
        }
        return false;
    }
};

template <typename Geometry>
struct geometry_type_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, 
                byte_order_type::enum_t order)
    {
        boost::uint32_t value;
        if (value_parser<boost::uint32_t>::parse(it, end, value, order))
        {
            return geometry_type<Geometry>::check(value);
        }
        return false;
    }
};

template <typename P,
          std::size_t I = 0,
          std::size_t N = dimension<P>::value>
struct parsing_assigner
{
    template <typename Iterator>
    static void run(Iterator& it, Iterator end, P& point, 
                byte_order_type::enum_t order)
    {
        typedef typename coordinate_type<P>::type coordinate_type;

        // coordinate type in WKB is always double
        double value(0);
        if (value_parser<double>::parse(it, end, value, order))
        {
            // actual coordinate type of point may be different
            set<I>(point, static_cast<coordinate_type>(value));
        }
        else
        {
            // TODO: mloskot - Report premature termination at coordinate level
            //throw failed to read coordinate value

            // default initialized value as fallback
            set<I>(point, coordinate_type());
        }
        parsing_assigner<P, I+1, N>::run(it, end, point, order);
    }
};

template <typename P, std::size_t N>
struct parsing_assigner<P, N, N>
{
    template <typename Iterator>
    static void run(Iterator& /*it*/, Iterator /*end*/, P& /*point*/,
                byte_order_type::enum_t /*order*/)
    {
        // terminate
    }
};

template <typename P>
struct point_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, P& point, 
                byte_order_type::enum_t order)
    {
        if (geometry_type_parser<P>::parse(it, end, order))
        {
            if (it != end)
            {
                parsing_assigner<P>::run(it, end, point, order);
            }
            return true;
        }
        return false;
    }
};

template <typename C>
struct point_container_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, C& container, 
                byte_order_type::enum_t order)
    {
        typedef typename point_type<C>::type point_type;

        boost::uint32_t num_points(0);
        if (!value_parser<boost::uint32_t>::parse(it, end, num_points, order))
        {
            return false;
        }

        typedef typename std::iterator_traits<Iterator>::difference_type size_type;
        BOOST_GEOMETRY_ASSERT(num_points <= boost::uint32_t( (std::numeric_limits<size_type>::max)() ) );

        size_type const container_size = static_cast<size_type>(num_points);
        size_type const point_size = dimension<point_type>::value * sizeof(double);

        if (std::distance(it, end) >= (container_size * point_size))
        {
            point_type point_buffer;

            // Read coordinates into point and append point to line (ring)
            size_type points_parsed = 0;
            while (points_parsed < container_size && it != end)
            {
                parsing_assigner<point_type>::run(it, end, point_buffer, order);
                boost::geometry::append(container, point_buffer);
                ++points_parsed;
            }

            if (container_size != points_parsed)
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }
};

template <typename L>
struct linestring_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, L& linestring, 
                byte_order_type::enum_t order)
    {
        if (!geometry_type_parser<L>::parse(it, end, order))
        {
            return false;
        }

        BOOST_GEOMETRY_ASSERT(it != end);
        return point_container_parser<L>::parse(it, end, linestring, order);
    }
};

template <typename Polygon>
struct polygon_parser
{
    template <typename Iterator>
    static bool parse(Iterator& it, Iterator end, Polygon& polygon, 
                byte_order_type::enum_t order)
    {
        if (!geometry_type_parser<Polygon>::parse(it, end, order))
        {
            return false;
        }

        boost::uint32_t num_rings(0);
        if (!value_parser<boost::uint32_t>::parse(it, end, num_rings, order))
        {
            return false;
        }

        typedef typename ring_type<Polygon>::type ring_type;

        std::size_t rings_parsed = 0;
        while (rings_parsed < num_rings && it != end)
        {
            if (0 == rings_parsed)
            {
                ring_type& ring0 = exterior_ring(polygon);
                if (!point_container_parser<ring_type>::parse(it, end, ring0, order))
                {
                    return false;
                }
            }
            else
            {
                interior_rings(polygon).resize(rings_parsed);
                ring_type& ringN = interior_rings(polygon).back();
                if (!point_container_parser<ring_type>::parse(it, end, ringN, order))
                {
                    return false;
                }
            }
            ++rings_parsed;
        }

        if (num_rings != rings_parsed)
        {
            return false;
        }

        return true;
    }
};

}} // namespace detail::wkb
#endif // DOXYGEN_NO_IMPL

}} // namespace boost::geometry
#endif // BOOST_GEOMETRY_IO_WKB_DETAIL_PARSER_HPP
