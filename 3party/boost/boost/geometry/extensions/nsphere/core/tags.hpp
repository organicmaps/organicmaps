// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_CORE_TAGS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_CORE_TAGS_HPP


namespace boost { namespace geometry
{

// TEMP: areal_tag commented out to prevent falling into the implementation for Areal geometries
//       in the new implementation of disjoint().
//       Besides this tag is invalid in the case of Dimension != 2

/// Convenience 2D (circle) or 3D (sphere) n-sphere identifying tag
struct nsphere_tag : single_tag/*, areal_tag*/ {};



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_CORE_TAGS_HPP
