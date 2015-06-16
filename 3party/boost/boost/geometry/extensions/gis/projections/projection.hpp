// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_PROJECTION_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PROJECTION_HPP


#include <string>
#include <boost/geometry/extensions/gis/projections/impl/projects.hpp>

namespace boost { namespace geometry { namespace projections
{

/*!
    \brief projection virtual base class
    \details class containing virtual methods
    \ingroup projection
    \tparam LL latlong point type
    \tparam XY xy point type
*/

template <typename LL, typename XY>
class projection
{
    protected :
        // see comment above
        //typedef typename geometry::coordinate_type<LL>::type LL_T;
        //typedef typename geometry::coordinate_type<XY>::type XY_T;
        typedef double LL_T;
        typedef double XY_T;

    public :

        typedef LL geographic_point_type; ///< latlong point type
        typedef XY cartesian_point_type;  ///< xy point type

        /// Forward projection, from Latitude-Longitude to Cartesian
        virtual bool forward(LL const& lp, XY& xy) const = 0;

        /// Inverse projection, from Cartesian to Latitude-Longitude
        virtual bool inverse(XY const& xy, LL& lp) const = 0;

        /// Forward projection using lon / lat and x / y separately
        virtual void fwd(LL_T& lp_lon, LL_T& lp_lat, XY_T& xy_x, XY_T& xy_y) const = 0;

        /// Inverse projection using x / y and lon / lat
        virtual void inv(XY_T& xy_x, XY_T& xy_y, LL_T& lp_lon, LL_T& lp_lat) const = 0;

        /// Returns name of projection
        virtual std::string name() const = 0;

        /// Returns parameters of projection
        virtual parameters const& params() const = 0;

        /// Returns mutable parameters of projection
        virtual parameters& mutable_params() = 0;

        virtual ~projection() {}

};

}}} // namespace boost::geometry::projections



#endif

