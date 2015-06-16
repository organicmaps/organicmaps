// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_MULTI_ALGORITHMS_DISSOLVE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_MULTI_ALGORITHMS_DISSOLVE_HPP


#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/union.hpp>

#include <boost/geometry/extensions/algorithms/dissolve.hpp>
#include <boost/geometry/extensions/algorithms/detail/overlay/dissolver.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace dissolve
{

template <typename Multi, typename GeometryOut>
struct dissolve_multi
{
    template <typename RescalePolicy, typename OutputIterator>
    static inline OutputIterator apply(Multi const& multi, RescalePolicy const& rescale_policy, OutputIterator out)
    {
        typedef typename boost::range_value<Multi>::type polygon_type;
        typedef typename boost::range_iterator<Multi const>::type iterator_type;

        // Step 1: dissolve all polygons in the multi-polygon, independantly
        std::vector<GeometryOut> step1;
        for (iterator_type it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            dissolve_ring_or_polygon
                <
                    polygon_type,
                    GeometryOut
                >::apply(*it, rescale_policy, std::back_inserter(step1));
        }

        // Step 2: remove mutual overlap
        {
            std::vector<GeometryOut> step2; // TODO avoid this, output to "out", if possible
            detail::dissolver::dissolver_generic<detail::dissolver::plusmin_policy>::apply(step1, rescale_policy, step2);
            for (typename std::vector<GeometryOut>::const_iterator it = step2.begin();
                it != step2.end(); ++it)
            {
                *out++ = *it;
            }
        }

        return out;
    }
};

// Dissolving multi-linestring is currently moved to extensions/algorithms/connect,
// because it is actually different from dissolving of polygons.
// To be decided what the final behaviour/name is.

}} // namespace detail::dissolve
#endif



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template<typename Multi, typename GeometryOut>
struct dissolve<multi_polygon_tag, polygon_tag, Multi, GeometryOut>
    : detail::dissolve::dissolve_multi<Multi, GeometryOut>
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_MULTI_ALGORITHMS_DISSOLVE_HPP
