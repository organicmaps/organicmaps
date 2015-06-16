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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_TYPE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_TYPE_HPP

#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/extensions/algebra/core/tags.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch {

template <typename Vector>
struct coordinate_type<vector_tag, Vector>
{
    typedef typename traits::coordinate_type<
        typename geometry::util::bare_type<Vector>::type
    >::type type;
};

template <typename G>
struct coordinate_type<quaternion_tag, G>
{
    typedef typename traits::coordinate_type<
        typename geometry::util::bare_type<G>::type
    >::type type;
};

template <typename G>
struct coordinate_type<matrix_tag, G>
{
    typedef typename traits::coordinate_type<
        typename geometry::util::bare_type<G>::type
    >::type type;
};


template <typename G>
struct coordinate_type<rotation_quaternion_tag, G>
{
    typedef typename traits::coordinate_type<
        typename geometry::util::bare_type<G>::type
    >::type type;
};

template <typename G>
struct coordinate_type<rotation_matrix_tag, G>
{
    typedef typename traits::coordinate_type<
        typename geometry::util::bare_type<G>::type
    >::type type;
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_TYPE_HPP
