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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_CONVERT_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_CONVERT_HPP

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/extensions/algebra/core/tags.hpp>

#include <boost/geometry/extensions/algebra/algorithms/detail.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename RQuaternion, typename RMatrix>
struct convert<RQuaternion, RMatrix, rotation_quaternion_tag, rotation_matrix_tag, 3, false>
{
    static inline void apply(RQuaternion const& q, RMatrix& m)
    {
        typedef typename coordinate_type<RQuaternion>::type T;

        // quaternion should be normalized

        T xx2 = get<1>(q) * get<1>(q) * 2;
        T yy2 = get<2>(q) * get<2>(q) * 2;
        T zz2 = get<3>(q) * get<3>(q) * 2;
        T xy2 = get<1>(q) * get<2>(q) * 2;
        T yz2 = get<2>(q) * get<3>(q) * 2;
        T xz2 = get<1>(q) * get<3>(q) * 2;
        T wx2 = get<0>(q) * get<1>(q) * 2;
        T wy2 = get<0>(q) * get<2>(q) * 2;
        T wz2 = get<0>(q) * get<3>(q) * 2;

        // WARNING!
        // Quaternion (0, 0, 0, 0) is converted to identity matrix!

        set<0, 0>(m, 1-yy2-zz2); set<0, 1>(m, xy2-wz2);   set<0, 2>(m, xz2+wy2);
        set<1, 0>(m, xy2+wz2);   set<1, 1>(m, 1-xx2-zz2); set<1, 2>(m, yz2-wx2);
        set<2, 0>(m, xz2-wy2);   set<2, 1>(m, yz2+wx2);   set<2, 2>(m, 1-xx2-yy2);
    }
};

template <typename RMatrix, typename RQuaternion>
struct convert<RMatrix, RQuaternion, rotation_matrix_tag, rotation_quaternion_tag, 3, false>
{
    static inline void apply(RMatrix const& m, RQuaternion & q)
    {
        typedef typename coordinate_type<RMatrix>::type T;

        // WARNING!
        // Zero matrix is converted to quaternion(0.5, 0, 0, 0)!

        T w = math::sqrt(1 + get<0, 0>(m) + get<1, 1>(m) + get<2, 2>(m)) / 2;
        T iw4 = 0.25 / w;
        set<0>(q, w);
        set<1>(q, (get<2, 1>(m) - get<1, 2>(m)) * iw4);
        set<2>(q, (get<0, 2>(m) - get<2, 0>(m)) * iw4);
        set<3>(q, (get<1, 0>(m) - get<0, 1>(m)) * iw4);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_ASSIGN_HPP
