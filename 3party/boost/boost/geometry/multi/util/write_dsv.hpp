// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_UTIL_WRITE_DSV_HPP
#define BOOST_GEOMETRY_MULTI_UTIL_WRITE_DSV_HPP

#include <boost/range.hpp>

#include <boost/geometry/util/write_dsv.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace dsv
{


template <typename MultiGeometry>
struct dsv_multi
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                MultiGeometry const& multi,
                dsv_settings const& settings)
    {
        os << settings.list_open;

        typedef typename boost::range_iterator
            <
                MultiGeometry const
            >::type iterator;
        for(iterator it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            os << geometry::dsv(*it);
        }
        os << settings.list_close;
    }
};




}} // namespace detail::dsv
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Geometry>
struct dsv<multi_tag, Geometry>
    : detail::dsv::dsv_multi<Geometry>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_UTIL_WRITE_DSV_HPP
