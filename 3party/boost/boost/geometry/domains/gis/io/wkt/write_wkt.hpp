// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_DOMAINS_GIS_IO_WKT_WRITE_WKT_HPP
#define BOOST_GEOMETRY_DOMAINS_GIS_IO_WKT_WRITE_WKT_HPP

#include <ostream>
#include <string>

#include <boost/array.hpp>
#include <boost/concept/assert.hpp>
#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>


#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/geometries/ring.hpp>

#include <boost/geometry/domains/gis/io/wkt/detail/wkt.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkt
{

template <typename P, int I, int Count>
struct stream_coordinate
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        os << (I > 0 ? " " : "") << get<I>(p);
        stream_coordinate<P, I + 1, Count>::apply(os, p);
    }
};

template <typename P, int Count>
struct stream_coordinate<P, Count, Count>
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>&, P const&)
    {}
};

struct prefix_linestring_par
{
    static inline const char* apply() { return "LINESTRING("; }
};

struct prefix_ring_par_par
{
    // Note, double parentheses are intentional, indicating WKT ring begin/end
    static inline const char* apply() { return "POLYGON(("; }
};

struct opening_parenthesis
{
    static inline const char* apply() { return "("; }
};

struct closing_parenthesis
{
    static inline const char* apply() { return ")"; }
};

struct double_closing_parenthesis
{
    static inline const char* apply() { return "))"; }
};




/*!
\brief Stream points as \ref WKT
*/
template <typename Point, typename Policy>
struct wkt_point
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os, Point const& p)
    {
        os << Policy::apply() << "(";
        stream_coordinate<Point, 0, dimension<Point>::type::value>::apply(os, p);
        os << ")";
    }
};

/*!
\brief Stream ranges as WKT
\note policy is used to stream prefix/postfix, enabling derived classes to override this
*/
template <typename Range, typename PrefixPolicy, typename SuffixPolicy>
struct wkt_range
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Range const& range)
    {
        typedef typename boost::range_iterator<Range const>::type iterator_type;

        bool first = true;

        os << PrefixPolicy::apply();

        // TODO: check EMPTY here

        for (iterator_type it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            os << (first ? "" : ",");
            stream_coordinate
                <
                    point_type, 0, dimension<point_type>::type::value
                >::apply(os, *it);
            first = false;
        }

        os << SuffixPolicy::apply();
    }

private:
    typedef typename boost::range_value<Range>::type point_type;
};

/*!
\brief Stream sequence of points as WKT-part, e.g. (1 2),(3 4)
\note Used in polygon, all multi-geometries
*/



template <typename Range>
struct wkt_sequence
    : wkt_range
        <
            Range,
            opening_parenthesis,
            closing_parenthesis
        >
{};


template <typename Polygon, typename PrefixPolicy>
struct wkt_poly
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Polygon const& poly)
    {
        typedef typename ring_type<Polygon const>::type ring;

        os << PrefixPolicy::apply();
        // TODO: check EMPTY here
        os << "(";
        wkt_sequence<ring>::apply(os, exterior_ring(poly));

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(poly);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            os << ",";
            wkt_sequence<ring>::apply(os, *it);
        }
        os << ")";
    }
};


template <typename Box>
struct wkt_box
{
    typedef typename point_type<Box>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Box const& box)
    {
        // Convert to ring, then stream
        typedef model::ring<point_type> ring_type;
        ring_type ring;
        geometry::convert(box, ring);
        os << "POLYGON(";
        wkt_sequence<ring_type>::apply(os, ring);
        os << ")";
    }

    private:

        inline wkt_box()
        {
            // Only streaming of boxes with two dimensions is support, otherwise it is a polyhedron!
            //assert_dimension<B, 2>();
        }
};


template <typename Segment>
struct wkt_segment
{
    typedef typename point_type<Segment>::type point_type;

    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Segment const& segment)
    {
        // Convert to two points, then stream
        typedef boost::array<point_type, 2> sequence;

        sequence points;
        geometry::detail::assign_point_from_index<0>(segment, points[0]);
        geometry::detail::assign_point_from_index<1>(segment, points[1]);

        // In Boost.Geometry a segment is represented
        // in WKT-format like (for 2D): LINESTRING(x y,x y)
        os << "LINESTRING";
        wkt_sequence<sequence>::apply(os, points);
    }

    private:

        inline wkt_segment()
        {}
};

}} // namespace detail::wkt
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Tag, typename Geometry>
struct wkt
{
   BOOST_MPL_ASSERT_MSG
        (
            false, NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (types<Geometry>)
        );
};


template <typename Point>
struct wkt<point_tag, Point>
    : detail::wkt::wkt_point
        <
            Point,
            detail::wkt::prefix_point
        >
{};


template <typename Linestring>
struct wkt<linestring_tag, Linestring>
    : detail::wkt::wkt_range
        <
            Linestring,
            detail::wkt::prefix_linestring_par,
            detail::wkt::closing_parenthesis
        >
{};


/*!
\brief Specialization to stream a box as WKT
\details A "box" does not exist in WKT.
It is therefore streamed as a polygon
*/
template <typename Box>
struct wkt<box_tag, Box>
    : detail::wkt::wkt_box<Box>
{};

template <typename Segment>
struct wkt<segment_tag, Segment>
    : detail::wkt::wkt_segment<Segment>
{};


/*!
\brief Specialization to stream a ring as WKT
\details A ring or "linear_ring" does not exist in WKT.
A ring is equivalent to a polygon without inner rings
It is therefore streamed as a polygon
*/
template <typename Ring>
struct wkt<ring_tag, Ring>
    : detail::wkt::wkt_range
        <
            Ring,
            detail::wkt::prefix_ring_par_par,
            detail::wkt::double_closing_parenthesis
        >
{};


/*!
\brief Specialization to stream polygon as WKT
*/
template <typename Polygon>
struct wkt<polygon_tag, Polygon>
    : detail::wkt::wkt_poly
        <
            Polygon,
            detail::wkt::prefix_polygon
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Generic geometry template manipulator class, takes corresponding output class from traits class
\ingroup wkt
\details Stream manipulator, streams geometry classes as \ref WKT streams
\par Example:
Small example showing how to use the wkt class
\dontinclude doxygen_1.cpp
\skip example_as_wkt_point
\line {
\until }
*/
template <typename Geometry>
class wkt_manipulator
{
public:

    inline wkt_manipulator(Geometry const& g)
        : m_geometry(g)
    {}

    template <typename Char, typename Traits>
    inline friend std::basic_ostream<Char, Traits>& operator<<(
            std::basic_ostream<Char, Traits>& os,
            wkt_manipulator const& m)
    {
        dispatch::wkt
            <
                typename tag<Geometry>::type,
                Geometry
            >::apply(os, m.m_geometry);
        os.flush();
        return os;
    }

private:
    Geometry const& m_geometry;
};

/*!
\brief Main WKT-streaming function
\ingroup wkt
\par Example:
Small example showing how to use the wkt helper function
\dontinclude doxygen_1.cpp
\skip example_as_wkt_vector
\line {
\until }
*/
template <typename Geometry>
inline wkt_manipulator<Geometry> wkt(Geometry const& geometry)
{
    concept::check<Geometry const>();

    return wkt_manipulator<Geometry>(geometry);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_DOMAINS_GIS_IO_WKT_WRITE_WKT_HPP
