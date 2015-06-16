// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_VESHAPE_WRITE_VESHAPE_HPP
#define BOOST_GEOMETRY_IO_VESHAPE_WRITE_VESHAPE_HPP

#include <ostream>
#include <string>

#include <boost/concept/assert.hpp>
#include <boost/range.hpp>


#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/point_concept.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace veshape
{


// Define the coordinate streamer, specialized for either 2 or 3 dimensions.
// Any other number of dimensions make no sense for VE, and we have to take care about
// the order lat,long (--> y,x)
template <typename P, std::size_t D>
struct stream_coordinate {};


template <typename P>
struct stream_coordinate<P, 2>
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        os << geometry::get<1>(p) << "," << geometry::get<0>(p);
    }
};

template <typename P>
struct stream_coordinate<P, 3>
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        stream_coordinate<P, 2>::stream(os, p);
        os << "," << geometry::get<2>(p);
    }
};


template <typename P>
struct stream_point
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        os << "new VELatLong(";
        stream_coordinate<P, dimension<P>::value>::stream(os, p);
        os << ")";
    }
};



struct prefix_point
{
    static inline const char* prefix()
    { return "new VEShape(VEShapeType.Pushpin, "; }

    static inline const char* postfix()
    { return ")"; }
};

struct prefix_linestring
{
    static inline const char* prefix()
    { return "new VEShape(VEShapeType.Polyline, "; }

    static inline const char* postfix()
    { return ")"; }
};


struct prefix_polygon
{
    static inline const char* prefix()
    { return "new VEShape(VEShapeType.Polygon, "; }

    static inline const char* postfix()
    { return ")"; }
};

/*!
\brief Stream points as \ref VEShape
*/
template <typename P, typename Policy>
struct veshape_point
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, P const& p)
    {
        os << Policy::prefix();
        stream_point<P>::stream(os, p);
        os << Policy::postfix();
    }

    private:
        BOOST_CONCEPT_ASSERT( (concept::ConstPoint<P>) );
};

/*!
\brief Stream ranges as VEShape
\note policy is used to stream prefix/postfix, enabling derived classes to override this
*/
template <typename R, typename Policy>
struct veshape_range
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, R const& range)
    {
        typedef typename boost::range_iterator<R const>::type iterator;

        bool first = true;

        os << Policy::prefix() << "new Array(";

        for (iterator it = boost::begin(range); it != boost::end(range); ++it)
        {
            os << (first ? "" : ", ");
            stream_point<point>::stream(os, *it);
            first = false;
        }

        os << ")" << Policy::postfix();
    }

    private:
        typedef typename boost::range_value<R>::type point;
        BOOST_CONCEPT_ASSERT( (concept::ConstPoint<point>) );
};



template <typename P, typename Policy>
struct veshape_poly
{
    template <typename Char, typename Traits>
    static inline void stream(std::basic_ostream<Char, Traits>& os, P const& poly)
    {
        typedef typename ring_type<P>::type ring;

        veshape_range<ring, Policy>::stream(os, exterior_ring(poly));

        // For VE shapes: inner rings are not supported or undocumented
        /***
        for (iterator it = boost::begin(interior_rings(poly));
            it != boost::end(interior_rings(poly)); it++)
        {
            os << ",";
            veshape_range<ring, null>::stream(os, *it);
        }
        os << ")";
        ***/
    }

    private:
        BOOST_CONCEPT_ASSERT( (concept::ConstPoint<typename point_type<P>::type>) );
};



}} // namespace detail::veshape
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

/*!
\brief Dispatching base struct for VEShape streaming, specialized below per geometry type
\details Specializations should implement a static method "stream" to stream a geometry
The static method should have the signature:

template <typename Char, typename Traits>
static inline void stream(std::basic_ostream<Char, Traits>& os, G const& geometry)
*/
template <typename T, typename G>
struct veshape
{};


template <typename P>
struct veshape<point_tag, P>
    : detail::veshape::veshape_point<P, detail::veshape::prefix_point>
{};


template <typename R>
struct veshape<linestring_tag, R>
    : detail::veshape::veshape_range<R, detail::veshape::prefix_linestring>
{};


template <typename R>
struct veshape<ring_tag, R>
    : detail::veshape::veshape_range<R, detail::veshape::prefix_polygon>
{};


template <typename P>
struct veshape<polygon_tag, P>
    : detail::veshape::veshape_poly<P, detail::veshape::prefix_polygon>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Generic geometry template manipulator class, takes corresponding output class from traits class
\ingroup veshape
\details Stream manipulator, streams geometry classes as Virtual Earth shape
*/
template <typename G>
class veshape_manip
{
public:

    inline veshape_manip(G const& g)
        : m_geometry(g)
    {}

    template <typename Char, typename Traits>
    inline friend std::basic_ostream<Char, Traits>& operator<<(
                    std::basic_ostream<Char, Traits>& os, veshape_manip const& m)
    {
        dispatch::veshape<typename tag<G>::type, G>::stream(os, m.m_geometry);
        os.flush();
        return os;
    }

private:
    G const& m_geometry;
};

/*!
\brief Object generator to conveniently stream objects without including streamveshape
\ingroup veshape
\par Example:
Small example showing how to use the veshape helper function
\dontinclude doxygen_1.cpp
\skip example_as_veshape_vector
\line {
\until }
*/
template <typename T>
inline veshape_manip<T> veshape(T const& t)
{
    return veshape_manip<T>(t);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_VESHAPE_WRITE_VESHAPE_HPP
