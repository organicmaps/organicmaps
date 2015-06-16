// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4

// Copyright (c) 2008-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file is converted from PROJ4, http://trac.osgeo.org/proj
// PROJ4 is originally written by Gerald Evenden (then of the USGS)
// PROJ4 is maintained by Frank Warmerdam
// PROJ4 is converted to Geometry Library by Barend Gehrels (Geodan, Amsterdam)

// Original copyright notice:

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP


#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail { namespace gauss {


static const int MAX_ITER = 20;

struct GAUSS
{
    double C;
    double K;
    double e;
    double ratexp;
};

static const double DEL_TOL = 1e-14;

inline double srat(double esinp, double exp)
{
    return (pow((1.0 - esinp) / (1.0 + esinp), exp));
}

inline GAUSS gauss_ini(double e, double phi0, double &chi, double &rc)
{
    using std::asin;
    using std::cos;
    using std::sin;
    using std::sqrt;
    using std::tan;

    double sphi = 0;
    double cphi = 0;
    double es = 0;

    GAUSS en;
    es = e * e;
    en.e = e;
    sphi = sin(phi0);
    cphi = cos(phi0);
    cphi *= cphi;

    rc = sqrt(1.0 - es) / (1.0 - es * sphi * sphi);
    en.C = sqrt(1.0 + es * cphi * cphi / (1.0 - es));
    chi = asin(sphi / en.C);
    en.ratexp = 0.5 * en.C * e;
    en.K = tan(0.5 * chi + detail::FORTPI)
           / (pow(tan(0.5 * phi0 + detail::FORTPI), en.C) * srat(en.e * sphi, en.ratexp));

    return en;
}

template <typename T>
inline void gauss(GAUSS const& en, T& lam, T& phi)
{
    phi = 2.0 * atan(en.K * pow(tan(0.5 * phi + FORTPI), en.C)
          * srat(en.e * sin(phi), en.ratexp) ) - geometry::math::half_pi<double>();

    lam *= en.C;
}

template <typename T>
inline void inv_gauss(GAUSS const& en, T& lam, T& phi)
{
    lam /= en.C;
    const double num = pow(tan(0.5 * phi + FORTPI) / en.K, 1.0 / en.C);

    int i = 0;
    for (i = MAX_ITER; i; --i)
    {
        const double elp_phi = 2.0 * atan(num * srat(en.e * sin(phi), - 0.5 * en.e)) - geometry::math::half_pi<double>();

        if (geometry::math::abs(elp_phi - phi) < DEL_TOL)
        {
            break;
        }
        phi = elp_phi;
    }

    /* convergence failed */
    if (!i)
    {
        throw proj_exception(-17);
    }
}

}} // namespace detail::gauss
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_GAUSS_HPP
