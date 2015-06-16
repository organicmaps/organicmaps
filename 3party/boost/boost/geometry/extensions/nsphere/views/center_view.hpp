// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_VIEWS_CENTER_VIEW_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_VIEWS_CENTER_VIEW_HPP

#include <boost/geometry/core/point_type.hpp>

namespace boost { namespace geometry
{

// Silence warning C4512: assignment operator could not be generated
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4512)
#endif

template <typename NSphere>
struct center_view
{
    typedef typename geometry::point_type<NSphere>::type point_type;
    typedef typename geometry::coordinate_type<point_type>::type coordinate_type;

    explicit center_view(NSphere & nsphere)
        : m_nsphere(nsphere)
    {}

    template <std::size_t I> coordinate_type get() const { return geometry::get<I>(m_nsphere); }
    template <std::size_t I> void set(coordinate_type const& v) { geometry::set<I>(m_nsphere, v); }

private :
    NSphere & m_nsphere;
};

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template<typename NSphere>
struct tag< center_view<NSphere> >
{
    typedef point_tag type;
};

template<typename NSphere>
struct coordinate_type< center_view<NSphere> >
{
    typedef typename geometry::coordinate_type<
        typename geometry::point_type<NSphere>::type
    >::type type;
};

template<typename NSphere>
struct coordinate_system< center_view<NSphere> >
{
    typedef typename geometry::coordinate_system<
        typename geometry::point_type<NSphere>::type
    >::type type;
};

template<typename NSphere>
struct dimension< center_view<NSphere> >
    : geometry::dimension< typename geometry::point_type<NSphere>::type >
{};

template<typename NSphere, std::size_t Dimension>
struct access<center_view<NSphere>, Dimension>
{
    typedef typename geometry::coordinate_type<
        typename geometry::point_type<NSphere>::type
    >::type coordinate_type;

    static inline coordinate_type get(center_view<NSphere> const& p)
    {
        return p.template get<Dimension>();
    }

    static inline void set(center_view<NSphere> & p, coordinate_type const& value)
    {
        p.template set<Dimension>(value);
    }
};

}
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_VIEWS_CENTER_VIEW_HPP
