// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_EXCEPTION_HPP
#define BOOST_GEOMETRY_CORE_EXCEPTION_HPP

#include <exception>

namespace boost { namespace geometry
{

/*!
\brief Base exception class for Boost.Geometry algorithms
\ingroup core
\details This class is never thrown. All exceptions thrown in Boost.Geometry
    are derived from exception, so it might be convenient to catch it.
*/
class exception : public std::exception
{};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_EXCEPTION_HPP
