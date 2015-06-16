// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGY_PROJECT_TRANSFORMER_HPP
#define BOOST_GEOMETRY_STRATEGY_PROJECT_TRANSFORMER_HPP


#include <boost/shared_ptr.hpp>

#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>
#include <boost/geometry/extensions/gis/projections/factory.hpp>
#include <boost/geometry/extensions/gis/projections/parameters.hpp>



namespace boost { namespace geometry { namespace projections
{
/*!
    \brief Transformation strategy to do transform using a Map Projection
    \ingroup transform
    \tparam LatLong first point type
    \tparam Cartesian second point type

    See also \link p03_projmap_example.cpp the projmap example \endlink
    where this last one plus a transformation using a projection are used.

 */
template <typename LatLong, typename Cartesian>
struct project_transformer
{
    typedef boost::shared_ptr<projection<LatLong, Cartesian> > projection_ptr;

    projection_ptr m_prj;

    inline project_transformer(projection_ptr& prj)
        : m_prj(prj)
    {}

    inline project_transformer(std::string const& par)
    {
        factory<LatLong, Cartesian, parameters> fac;
        m_prj.reset(fac.create_new(init(par)));
    }

    inline bool apply(LatLong const& p1, Cartesian& p2) const
    {
        // Latlong (LatLong -> Cartesian) will be projected, rest will be copied.
        // So first copy third or higher dimensions
        geometry::detail::conversion::point_to_point<LatLong, Cartesian, 2,
                geometry::dimension<Cartesian>::value> ::apply(p1, p2);
        return m_prj->forward(p1, p2);
    }

};

}}} // namespace boost::geometry::projections


#endif // BOOST_GEOMETRY_STRATEGY_PROJECT_TRANSFORMER_HPP
