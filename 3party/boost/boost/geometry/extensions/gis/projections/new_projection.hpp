// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012 Krzysztof Czainski
// Copyright (c) 2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_PROJECTIONS_NEW_PROJECTION_HPP
#define BOOST_GEOMETRY_PROJECTIONS_NEW_PROJECTION_HPP

#include <boost/geometry/extensions/gis/projections/impl/base_dynamic.hpp>
#include <boost/geometry/extensions/gis/projections/projection_point_type.hpp>
#include <boost/geometry/extensions/gis/projections/projection.hpp>

namespace boost { namespace geometry { namespace projections
{

/*!
\brief  Creates a type-erased projection
\details  Creates using operator new a class derived from projection, that forwards method
          calls to @p Proj.
\ingroup projection
\tparam Projection  Type of the concrete projection to be created.
\tparam Parameters  projection parameters type
\see  projection
\see  factory
*/

//@{
template <typename Projection, typename Parameters>
inline projection
        <
            typename detail::projection_point_type<Projection, geographic_tag>::type
          , typename detail::projection_point_type<Projection, cartesian_tag>::type
        >* new_projection(Parameters const& par)
{
    return new detail::base_v_fi
        <
            Projection
            , typename detail::projection_point_type<Projection, geographic_tag>::type
            , typename detail::projection_point_type<Projection, cartesian_tag>::type
            , Parameters
        >(par);
}
//@}

}}} // boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_NEW_PROJECTION_HPP
