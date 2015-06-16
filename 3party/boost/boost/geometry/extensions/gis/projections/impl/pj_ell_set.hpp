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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP

#include <string>
#include <vector>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/extensions/gis/projections/impl/pj_ellps.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_param.hpp>

namespace boost { namespace geometry { namespace projections {

namespace detail {

/* set ellipsoid parameters a and es */
static const double SIXTH =  .1666666666666666667; /* 1/6 */
static const double RA4 = .04722222222222222222; /* 17/360 */
static const double RA6 = .02215608465608465608; /* 67/3024 */
static const double RV4 = .06944444444444444444; /* 5/72 */
static const double RV6 = .04243827160493827160; /* 55/1296 */

/* initialize geographic shape parameters */
inline void pj_ell_set(std::vector<pvalue>& parameters, double &a, double &es)
{
    double b = 0.0;
    double e = 0.0;
    std::string name;

    /* check for varying forms of ellipsoid input */
    a = es = 0.;

    /* R takes precedence */
    if (pj_param(parameters, "tR").i)
        a = pj_param(parameters, "dR").f;
    else { /* probable elliptical figure */

        /* check if ellps present and temporarily append its values to pl */
        name = pj_param(parameters, "sellps").s;
        if (! name.empty())
        {
            const int n = sizeof(pj_ellps) / sizeof(pj_ellps[0]);
            int index = -1;
            for (int i = 0; i < n && index == -1; i++)
            {
                if(pj_ellps[i].id == name)
                {
                    index = i;
                }
            }

            if (index == -1) { throw proj_exception(-9); }

            parameters.push_back(pj_mkparam(pj_ellps[index].major));
            parameters.push_back(pj_mkparam(pj_ellps[index].ell));
        }
        a = pj_param(parameters, "da").f;
        if (pj_param(parameters, "tes").i) /* eccentricity squared */
            es = pj_param(parameters, "des").f;
        else if (pj_param(parameters, "te").i) { /* eccentricity */
            e = pj_param(parameters, "de").f;
            es = e * e;
        } else if (pj_param(parameters, "trf").i) { /* recip flattening */
            es = pj_param(parameters, "drf").f;
            if (!es) {
                throw proj_exception(-10);
            }
            es = 1./ es;
            es = es * (2. - es);
        } else if (pj_param(parameters, "tf").i) { /* flattening */
            es = pj_param(parameters, "df").f;
            es = es * (2. - es);
        } else if (pj_param(parameters, "tb").i) { /* minor axis */
            b = pj_param(parameters, "db").f;
            es = 1. - (b * b) / (a * a);
        }     /* else es == 0. and sphere of radius a */
        if (!b)
            b = a * sqrt(1. - es);
        /* following options turn ellipsoid into equivalent sphere */
        if (pj_param(parameters, "bR_A").i) { /* sphere--area of ellipsoid */
            a *= 1. - es * (SIXTH + es * (RA4 + es * RA6));
            es = 0.;
        } else if (pj_param(parameters, "bR_V").i) { /* sphere--vol. of ellipsoid */
            a *= 1. - es * (SIXTH + es * (RV4 + es * RV6));
            es = 0.;
        } else if (pj_param(parameters, "bR_a").i) { /* sphere--arithmetic mean */
            a = .5 * (a + b);
            es = 0.;
        } else if (pj_param(parameters, "bR_g").i) { /* sphere--geometric mean */
            a = sqrt(a * b);
            es = 0.;
        } else if (pj_param(parameters, "bR_h").i) { /* sphere--harmonic mean */
            a = 2. * a * b / (a + b);
            es = 0.;
        } else {
            int i = pj_param(parameters, "tR_lat_a").i;
            if (i || /* sphere--arith. */
                pj_param(parameters, "tR_lat_g").i) { /* or geom. mean at latitude */
                double tmp;

                tmp = sin(pj_param(parameters, i ? "rR_lat_a" : "rR_lat_g").f);
                if (geometry::math::abs(tmp) > geometry::math::half_pi<double>()) {
                    throw proj_exception(-11);
                }
                tmp = 1. - es * tmp * tmp;
                a *= i ? .5 * (1. - es + tmp) / ( tmp * sqrt(tmp)) :
                    sqrt(1. - es) / tmp;
                es = 0.;
            }
        }
    }

    /* some remaining checks */
    if (es < 0.)
        { throw proj_exception(-12); }
    if (a <= 0.)
        { throw proj_exception(-13); }
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_ELL_SET_HPP
