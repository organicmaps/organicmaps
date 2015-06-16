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

#ifndef BOOST_GEOMETRY_PROJECTIONS_PROJ_MDIST_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PROJ_MDIST_HPP


#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry { namespace projections
{
namespace detail
{
    static const int MDIST_MAX_ITER = 20;

    struct MDIST
    {
        int nb;
        double es;
        double E;
        double b[MDIST_MAX_ITER];
    };

    inline bool proj_mdist_ini(double es, MDIST& b)
    {
        double numf, numfi, twon1, denf, denfi, ens, T, twon;
        double den, El, Es;
        double E[MDIST_MAX_ITER];
        int i, j;

        /* generate E(e^2) and its terms E[] */
        ens = es;
        numf = twon1 = denfi = 1.;
        denf = 1.;
        twon = 4.;
        Es = El = E[0] = 1.;
        for (i = 1; i < MDIST_MAX_ITER ; ++i)
        {
            numf *= (twon1 * twon1);
            den = twon * denf * denf * twon1;
            T = numf/den;
            Es -= (E[i] = T * ens);
            ens *= es;
            twon *= 4.;
            denf *= ++denfi;
            twon1 += 2.;
            if (Es == El) /* jump out if no change */
                break;
            El = Es;
        }
        b.nb = i - 1;
        b.es = es;
        b.E = Es;
        /* generate b_n coefficients--note: collapse with prefix ratios */
        b.b[0] = Es = 1. - Es;
        numf = denf = 1.;
        numfi = 2.;
        denfi = 3.;
        for (j = 1; j < i; ++j)
        {
            Es -= E[j];
            numf *= numfi;
            denf *= denfi;
            b.b[j] = Es * numf / denf;
            numfi += 2.;
            denfi += 2.;
        }
        return true;
    }
    inline double proj_mdist(double phi, double sphi, double cphi, const MDIST& b)
    {
        double sc, sum, sphi2, D;
        int i;

        sc = sphi * cphi;
        sphi2 = sphi * sphi;
        D = phi * b.E - b.es * sc / sqrt(1. - b.es * sphi2);
        sum = b.b[i = b.nb];
        while (i) sum = b.b[--i] + sphi2 * sum;
        return(D + sc * sum);
    }
    inline double proj_inv_mdist(double dist, const MDIST& b)
    {
        static const double TOL = 1e-14;
        double s, t, phi, k;
        int i;

        k = 1./(1.- b.es);
        i = MDIST_MAX_ITER;
        phi = dist;
        while ( i-- ) {
            s = sin(phi);
            t = 1. - b.es * s * s;
            phi -= t = (proj_mdist(phi, s, cos(phi), b) - dist) *
                (t * sqrt(t)) * k;
            if (geometry::math::abs(t) < TOL) /* that is no change */
                return phi;
        }
            /* convergence failed */
        throw proj_exception(-17);
    }
} // namespace detail

}}} // namespace boost::geometry::projections

#endif
