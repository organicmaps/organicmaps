// Boost.Geometry
//
// Copyright (c) 2015 Mats Taraldsvik.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKB_DETAIL_WRITER_HPP
#define BOOST_GEOMETRY_IO_WKB_DETAIL_WRITER_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>

#include <boost/concept_check.hpp>
#include <boost/cstdint.hpp>
#include <boost/range.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/geometry/core/access.hpp>
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
    struct value_writer
    {
        typedef T value_type;

        template <typename OutputIterator>
        static bool write(T const& value,
                          OutputIterator& iter,
                          byte_order_type::enum_t byte_order)
        {
            endian::endian_value<T> parsed_value(value);

// 				if (byte_order_type::xdr == byte_order)
// 				{
// 					parsed_value.template store<endian::big_endian_tag>(iter);
// 				}
// 				else if (byte_order_type::ndr == byte_order)
// 				{
// 					parsed_value.template store<endian::little_endian_tag>(iter);
// 				}
// 				else
// 				{
                parsed_value.template store<endian::native_endian_tag>(iter);
// 				}

            return true;
        }
    };

    template
    <
        typename Point,
        std::size_t I = 0,
        std::size_t N = dimension<Point>::value
    >
    struct writer_assigner
    {
        template <typename OutputIterator>
        static void run(Point const& point,
                        OutputIterator& iter,
                        byte_order_type::enum_t byte_order)
        {
            // NOTE: coordinates of any type are converted to double

            value_writer<double>::write(geometry::get<I>(point),
                                        iter,
                                        byte_order);

            writer_assigner<Point, I+1, N>::run(point, iter, byte_order);
        }
    };

    template <typename Point, std::size_t N>
    struct writer_assigner<Point, N, N>
    {
        template <typename OutputIterator>
        static void run(Point const& /*point*/,
                        OutputIterator& /*iter*/,
                        byte_order_type::enum_t /*byte_order*/)
        {
            // terminate
        }
    };

    template <typename Point>
    struct point_writer
    {
        template <typename OutputIterator>
        static bool write(Point const& point,
                          OutputIterator& iter,
                          byte_order_type::enum_t byte_order)
        {
            // write endian type
            value_writer<uint8_t>::write(byte_order, iter, byte_order);

            // write geometry type
            uint32_t type = geometry_type<Point>::get();
            value_writer<uint32_t>::write(type, iter, byte_order);

            // write point's x, y, z
            writer_assigner<Point>::run(point, iter, byte_order);

            return true;
        }
    };

    template <typename Linestring>
    struct linestring_writer
    {
        template <typename OutputIterator>
        static bool write(Linestring const& linestring,
                          OutputIterator& iter,
                          byte_order_type::enum_t byte_order)
        {
            // write endian type
            value_writer<uint8_t>::write(byte_order, iter, byte_order);

            // write geometry type
            uint32_t type = geometry_type<Linestring>::get();
            value_writer<uint32_t>::write(type, iter, byte_order);

            // write num points
            uint32_t num_points = boost::size(linestring);
            value_writer<uint32_t>::write(num_points, iter, byte_order);

            for(typename boost::range_iterator<Linestring const>::type
                    point_iter = boost::begin(linestring);
                point_iter != boost::end(linestring);
                ++point_iter)
            {
                // write point's x, y, z
                writer_assigner<typename point_type<Linestring>::type>
                    ::run(*point_iter, iter, byte_order);
            }

            return true;
        }
    };

    template <typename Polygon>
    struct polygon_writer
    {
        template <typename OutputIterator>
        static bool write(Polygon const& polygon,
                          OutputIterator& iter,
                          byte_order_type::enum_t byte_order)
        {
            // write endian type
            value_writer<uint8_t>::write(byte_order, iter, byte_order);

            // write geometry type
            uint32_t type = geometry_type<Polygon>::get();
            value_writer<uint32_t>::write(type, iter, byte_order);

            // write num rings
            uint32_t num_rings = 1 + geometry::num_interior_rings(polygon);
            value_writer<uint32_t>::write(num_rings, iter, byte_order);

            // write exterior ring
            typedef typename geometry::ring_type<Polygon const>::type
                ring_type;

            typename geometry::ring_return_type<Polygon const>::type
                exterior_ring = geometry::exterior_ring(polygon);

            value_writer<uint32_t>::write(geometry::num_points(exterior_ring),
                                          iter,
                                          byte_order);

            for(typename boost::range_iterator<ring_type const>::type
                    point_iter = boost::begin(exterior_ring);
                point_iter != boost::end(exterior_ring);
                ++point_iter)
            {
                // write point's x, y, z
                writer_assigner<typename point_type<Polygon>::type>
                    ::run(*point_iter, iter, byte_order);
            }

            // write interor rings
            typedef typename geometry::interior_type<Polygon const>::type
                interior_rings_type;

            typename geometry::interior_return_type<Polygon const>::type
                interior_rings = geometry::interior_rings(polygon);

            for(typename boost::range_iterator<interior_rings_type const>::type
                    ring_iter = boost::begin(interior_rings);
                ring_iter != boost::end(interior_rings);
                ++ring_iter)
            {
                value_writer<uint32_t>::write(geometry::num_points(*ring_iter),
                                              iter,
                                              byte_order);

                for(typename boost::range_iterator<ring_type const>::type
                        point_iter = boost::begin(*ring_iter);
                    point_iter != boost::end(*ring_iter); 
                    ++point_iter)
                {
                    // write point's x, y, z
                    writer_assigner<typename point_type<Polygon>::type>
                        ::run(*point_iter, iter, byte_order);
                }
            }

            return true;
        }
    };

}} // namespace detail::wkb
#endif // DOXYGEN_NO_IMPL

}} // namespace boost::geometry
#endif // BOOST_GEOMETRY_IO_WKB_DETAIL_WRITER_HPP
