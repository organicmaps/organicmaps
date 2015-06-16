// Boost.Geometry
//
// Copyright (c) 2015 Mats Taraldsvik.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKB_WRITE_WKB_HPP
#define BOOST_GEOMETRY_IO_WKB_WRITE_WKB_HPP

#include <iterator>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/static_assert.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/extensions/gis/io/wkb/detail/writer.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Tag, typename G>
struct write_wkb 
{
};

template <typename G>
struct write_wkb<point_tag, G>
{
    template <typename OutputIterator>
    static inline bool write(const G& geometry, OutputIterator iter,
                       detail::wkb::byte_order_type::enum_t byte_order)
    {
        return detail::wkb::point_writer<G>::write(geometry, iter, byte_order);
    }
};

template <typename G>
struct write_wkb<linestring_tag, G>
{
    template <typename OutputIterator>
    static inline bool write(const G& geometry, OutputIterator iter,
                       detail::wkb::byte_order_type::enum_t byte_order)
    {
        return detail::wkb::linestring_writer<G>::write(geometry, iter, byte_order);
    }
};

template <typename G>
struct write_wkb<polygon_tag, G>
{
    template <typename OutputIterator>
    static inline bool write(const G& geometry, OutputIterator iter,
                       detail::wkb::byte_order_type::enum_t byte_order)
    {
        return detail::wkb::polygon_writer<G>::write(geometry, iter, byte_order);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

template <typename G, typename OutputIterator>
inline bool write_wkb(const G& geometry, OutputIterator iter)
{
    // The WKB is written to an OutputIterator.
    BOOST_STATIC_ASSERT((
        boost::is_convertible
        <
        typename std::iterator_traits<OutputIterator>::iterator_category,
        const std::output_iterator_tag&
        >::value));

// Will write in the native byte order
#ifdef BOOST_BIG_ENDIAN
        detail::wkb::byte_order_type::enum_t byte_order =  detail::wkb::byte_order_type::xdr;
#else
        detail::wkb::byte_order_type::enum_t byte_order =  detail::wkb::byte_order_type::ndr;
#endif

    if
        (!dispatch::write_wkb
        <
        typename tag<G>::type,
        G
        >::write(geometry, iter, byte_order))
    {
        return false;
    }

    return true;
}

// 	template <typename G, typename OutputIterator>
// 	inline bool write_wkb(G& geometry, OutputIterator iter, 
// 		detail::wkb::byte_order_type::enum_t source_byte_order, 
// 		detail::wkb::byte_order_type::enum_t target_byte_order)
// 	{
// 		// The WKB is written to an OutputIterator.
// 		BOOST_STATIC_ASSERT((
// 			boost::is_convertible
// 			<
// 			typename std::iterator_traits<OutputIterator>::iterator_category,
// 			const std::output_iterator_tag&
// 			>::value));
// 
// 		if
// 			(
// 			!dispatch::write_wkb
// 			<
// 			typename tag<G>::type,
// 			G
// 			>::write(geometry, iter, byte_order)
// 			)
// 		{
// 			return false;
// 		}
// 
// 		return true;
// 	}

}} // namespace boost::geometry
#endif // BOOST_GEOMETRY_IO_WKB_WRITE_WKB_HPP
