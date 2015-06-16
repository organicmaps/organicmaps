// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_PROJECT_INVERSE_TRANSFORMER_HPP
#define BOOST_GEOMETRY_STRATEGY_PROJECT_INVERSE_TRANSFORMER_HPP


#include <boost/shared_ptr.hpp>

#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/extensions/gis/projections/factory.hpp>
#include <boost/geometry/extensions/gis/projections/parameters.hpp>


namespace boost { namespace geometry { namespace projections
{


/*!
    \brief Transformation strategy to do transform using a Map Projection
    \ingroup transform
    \tparam Cartesian first point type
    \tparam LatLong second point type
 */
template <typename Cartesian, typename LatLong>
struct project_inverse_transformer
{
    typedef boost::shared_ptr<projection<LatLong, Cartesian> > projection_ptr;

    projection_ptr m_prj;

    /// Constructor using a shared-pointer-to-projection_ptr
    inline project_inverse_transformer(projection_ptr& prj)
        : m_prj(prj)
    {}

    /// Constructor using a string
    inline project_inverse_transformer(std::string const& par)
    {
        factory<LatLong, Cartesian, parameters> fac;
        m_prj.reset(fac.create_new(init(par)));
    }

    /// Constructor using Parameters
    template <typename Parameters>
    inline project_inverse_transformer(Parameters const& par)
    {
        factory<LatLong, Cartesian, Parameters> fac;
        m_prj.reset(fac.create_new(par));
    }

    /// Transform operator
    inline bool apply(Cartesian const& p1, LatLong& p2) const
    {
        // Latlong (LL -> XY) will be projected, rest will be copied.
        // So first copy third or higher dimensions
        geometry::detail::conversion::point_to_point<Cartesian, LatLong, 2,
                geometry::dimension<Cartesian>::value> ::apply(p1, p2);
        return m_prj->inverse(p1, p2);
    }

};

}}} // namespace boost::geometry::projections


#endif // BOOST_GEOMETRY_STRATEGY_PROJECT_INVERSE_TRANSFORMER_HPP
