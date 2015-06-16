// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_CORE_RADIUS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_CORE_RADIUS_HPP


#include <cstddef>


#include <boost/geometry/core/radius.hpp>

#include <boost/geometry/extensions/nsphere/core/tags.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename S>
struct radius_type<nsphere_tag, S>
{
    typedef typename traits::radius_type<S>::type type;
};

template <typename S, std::size_t D>
struct radius_access<nsphere_tag, S, D, boost::false_type>
    : detail::radius_access<nsphere_tag, S, D>
{
    BOOST_STATIC_ASSERT((D == 0));
};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_RADIUS_HPP
