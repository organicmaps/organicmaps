// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OFFSET_APPENDER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OFFSET_APPENDER_HPP


#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace offset
{

// Appends points to an output range (linestring/ring).
template
    <
        typename Range
#ifdef BOOST_GEOMETRY_DEBUG_WITH_MAPPER
        , typename Mapper
#endif
    >
struct offset_appender
{
    typedef Range range_type;

    typedef typename geometry::point_type<Range>::type point_type;

#ifdef BOOST_GEOMETRY_DEBUG_WITH_MAPPER
    Mapper const& m_mapper;
    inline offset_appender(Range& r, Mapper const& mapper)
        : m_range(r)
        , m_mapper(mapper)
#else
    inline offset_appender(Range& r)
        : m_range(r)
#endif

    {}

    inline void append(point_type const& point)
    {
        do_append(point);
    }

    inline void append_begin_join(point_type const& point)
    {
        do_append(point);
    }

    inline void append_end_join(point_type const& point)
    {
        do_append(point);
    }

    inline void append_begin_hooklet(point_type const& point)
    {
        do_append(point);
    }

    inline void append_end_hooklet(point_type const& point)
    {
        do_append(point);
    }


private :

    Range& m_range;

    inline void do_append(point_type const& point)
    {
        m_range.push_back(point);
    }
};


}} // namespace detail::offset
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OFFSET_APPENDER_HPP
