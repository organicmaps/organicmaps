// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_FUNCTION_OVERLOADS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_FUNCTION_OVERLOADS_HPP

#include <cmath>

namespace boost { namespace geometry { namespace projections
{

// Functions to resolve ambiguity when compiling with coordinates of different types
/*inline double atan2(double a, double b)
{
    return std::atan2(a, b);
}
inline double pow(double a, double b)
{
    return std::pow(a, b);
}
*/

inline int int_floor(double f)
{
    return int(std::floor(f));
}

}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_FUNCTION_OVERLOADS_HPP
