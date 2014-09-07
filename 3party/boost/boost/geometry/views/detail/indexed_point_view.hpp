// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2014 Adam Wulkiewicz, Lodz, Poland

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_VIEWS_DETAIL_INDEXED_POINT_VIEW_HPP
#define BOOST_GEOMETRY_VIEWS_DETAIL_INDEXED_POINT_VIEW_HPP

#include <cstddef>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace detail
{

template <typename Geometry, std::size_t Index>
class indexed_point_view
{
    indexed_point_view & operator=(indexed_point_view const&);

public:
    typedef typename geometry::point_type<Geometry>::type point_type;
    typedef typename geometry::coordinate_type<Geometry>::type coordinate_type;

    indexed_point_view(Geometry & geometry)
        : m_geometry(geometry)
    {}

    template <std::size_t Dimension>
    inline coordinate_type get() const
    {
        return geometry::get<Index, Dimension>(m_geometry);
    }

    template <std::size_t Dimension>
    inline void set(coordinate_type const& value)
    {
        geometry::set<Index, Dimension>(m_geometry, value);
    }

private:
    Geometry & m_geometry;
};

}

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Geometry, std::size_t Index>
struct tag< detail::indexed_point_view<Geometry, Index> >
{
    typedef point_tag type;
};

template <typename Geometry, std::size_t Index>
struct coordinate_type< detail::indexed_point_view<Geometry, Index> >
{
    typedef typename geometry::coordinate_type<Geometry>::type type;
};

template <typename Geometry, std::size_t Index>
struct coordinate_system< detail::indexed_point_view<Geometry, Index> >
{
    typedef typename geometry::coordinate_system<Geometry>::type type;
};

template <typename Geometry, std::size_t Index>
struct dimension< detail::indexed_point_view<Geometry, Index> >
    : geometry::dimension<Geometry>
{};

template<typename Geometry, std::size_t Index, std::size_t Dimension>
struct access< detail::indexed_point_view<Geometry, Index>, Dimension >
{
    typedef typename geometry::coordinate_type<Geometry>::type coordinate_type;

    static inline coordinate_type get(
        detail::indexed_point_view<Geometry, Index> const& p)
    {
        return p.template get<Dimension>();
    }

    static inline void set(
        detail::indexed_point_view<Geometry, Index> & p,
        coordinate_type const& value)
    {
        p.template set<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_VIEWS_DETAIL_INDEXED_POINT_VIEW_HPP
