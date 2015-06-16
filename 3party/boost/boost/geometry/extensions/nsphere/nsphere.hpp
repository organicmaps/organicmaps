// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_HPP

#include <boost/geometry/extensions/nsphere/core/access.hpp>
#include <boost/geometry/extensions/nsphere/core/geometry_id.hpp>
#include <boost/geometry/extensions/nsphere/core/radius.hpp>
#include <boost/geometry/extensions/nsphere/core/replace_point_type.hpp>
#include <boost/geometry/extensions/nsphere/core/tags.hpp>
#include <boost/geometry/extensions/nsphere/core/topological_dimension.hpp>

#include <boost/geometry/extensions/nsphere/geometries/concepts/check.hpp>
#include <boost/geometry/extensions/nsphere/geometries/concepts/nsphere_concept.hpp>

#include <boost/geometry/extensions/nsphere/geometries/nsphere.hpp>

#include <boost/geometry/extensions/nsphere/algorithms/append.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/area.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/assign.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/clear.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/envelope.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/num_points.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/disjoint.hpp>
#include <boost/geometry/extensions/nsphere/strategies/cartesian/nsphere_in_box.hpp>
#include <boost/geometry/extensions/nsphere/strategies/cartesian/point_in_nsphere.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/within.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/covered_by.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/expand.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/equals.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/centroid.hpp>

#include <boost/geometry/extensions/nsphere/index/indexable.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/algorithms/content.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/algorithms/is_valid.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/algorithms/margin.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/algorithms/comparable_distance_near.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/algorithms/bounds.hpp>

#include <boost/geometry/index/detail/exception.hpp> // needed by the following
#include <boost/geometry/index/detail/rtree/options.hpp> // needed by the following
#include <boost/geometry/index/detail/translator.hpp> // needed by the following
#include <boost/geometry/extensions/nsphere/index/detail/rtree/linear/redistribute_elements.hpp>
#include <boost/geometry/extensions/nsphere/index/detail/rtree/rstar/redistribute_elements.hpp>

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_HPP
