// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGEBRA_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGEBRA_HPP

#include <boost/geometry/extensions/algebra/core/access.hpp>
#include <boost/geometry/extensions/algebra/core/coordinate_dimension.hpp>
#include <boost/geometry/extensions/algebra/core/coordinate_system.hpp>
#include <boost/geometry/extensions/algebra/core/coordinate_type.hpp>
#include <boost/geometry/extensions/algebra/core/tags.hpp>

#include <boost/geometry/extensions/algebra/geometries/concepts/vector_concept.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/rotation_quaternion_concept.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/rotation_matrix_concept.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/check.hpp>

#include <boost/geometry/extensions/algebra/geometries/vector.hpp>
#include <boost/geometry/extensions/algebra/geometries/rotation_quaternion.hpp>
#include <boost/geometry/extensions/algebra/geometries/rotation_matrix.hpp>

#include <boost/geometry/extensions/algebra/algorithms/assign.hpp>
#include <boost/geometry/extensions/algebra/algorithms/convert.hpp>

// experimental
#include <boost/geometry/extensions/algebra/algorithms/clear.hpp>
#include <boost/geometry/extensions/algebra/algorithms/reverse.hpp>

#include <boost/geometry/extensions/algebra/algorithms/translation.hpp>
#include <boost/geometry/extensions/algebra/algorithms/rotation.hpp>

// should be removed, transform() should be used instead
#include <boost/geometry/extensions/algebra/algorithms/transform_geometrically.hpp>

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGEBRA_HPP
