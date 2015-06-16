// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_PARAMETERS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PARAMETERS_HPP


#include <string>
#include <vector>


#include <boost/geometry/extensions/gis/projections/impl/pj_init.hpp>
#include <boost/geometry/extensions/gis/projections/impl/projects.hpp>


namespace boost { namespace geometry { namespace projections {

template <typename R>
inline parameters init(const R& arguments)
{
    return detail::pj_init(arguments);
}

/*!
\ingroup projection
\brief Initializes a projection as a string, using the format with + and =
\details The projection can be initialized with a string (with the same format as the PROJ4 package) for
  convenient initialization from, for example, the command line
\par Example
    <tt>+proj=labrd +ellps=intl +lon_0=46d26'13.95E +lat_0=18d54S +azi=18d54 +k_0=.9995 +x_0=400000 +y_0=800000</tt>
    for the Madagascar projection.
\note Parameters are described in the group
*/
inline parameters init(const std::string& arguments)
{
    return detail::pj_init_plus(arguments);
}

/*!
\ingroup projection
\brief Overload using a const char*
*/
inline parameters init(const char* arguments)
{
    return detail::pj_init_plus(arguments);
}


// todo:
/*
parameters init(const std::map<std::string, std::string>& arguments)
{
    return detail::pj_init_plus(arguments);
}
*/



}}} // namespace boost::geometry::projections
#endif
