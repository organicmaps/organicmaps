// Boost.Geometry (aka GGL, Generic Geometry Library)
// This file is manually converted from PROJ4 (projects.h)

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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PROJECTS_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PROJECTS_HPP

#include <cstring>
#include <string>
#include <vector>

#include <boost/math/constants/constants.hpp>

namespace boost { namespace geometry { namespace projections
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

/* some useful constants */
static const double FORTPI = boost::math::constants::pi<double>() / 4.0;

static const int PJD_UNKNOWN =0;
static const int PJD_3PARAM = 1;
static const int PJD_7PARAM = 2;
static const int PJD_GRIDSHIFT = 3;
static const int PJD_WGS84 = 4;   /* WGS84 (or anything considered equivelent) */


struct pvalue
{
    std::string param;
    int used;

    int i;
    double f;
    std::string s;
};

struct pj_const_pod
{
    int over;   /* over-range flag */
    int geoc;   /* geocentric latitude flag */
    int is_latlong; /* proj=latlong ... not really a projection at all */
    int is_geocent; /* proj=geocent ... not really a projection at all */
    double
        a,  /* major axis or radius if es==0 */
        a_orig, /* major axis before any +proj related adjustment */
        es, /* e ^ 2 */
        es_orig, /* es before any +proj related adjustment */
        e,  /* eccentricity */
        ra, /* 1/A */
        one_es, /* 1 - e^2 */
        rone_es, /* 1/one_es */
        lam0, phi0, /* central longitude, latitude */
        x0, y0, /* easting and northing */
        k0,    /* general scaling factor */
        to_meter, fr_meter; /* cartesian scaling */

    int datum_type; /* PJD_UNKNOWN/3PARAM/7PARAM/GRIDSHIFT/WGS84 */
    double  datum_params[7];
    double  from_greenwich; /* prime meridian offset (in radians) */
    double  long_wrap_center; /* 0.0 for -180 to 180, actually in radians*/

    // Initialize all variables to zero
    pj_const_pod()
    {
        std::memset(this, 0, sizeof(pj_const_pod));
    }
};

// PROJ4 complex. Might be replaced with std::complex
struct COMPLEX { double r, i; };

struct PJ_ELLPS
{
    std::string id;    /* ellipse keyword name */
    std::string major;    /* a= value */
    std::string ell;    /* elliptical parameter */
    std::string name;    /* comments */
};

struct PJ_DATUMS
{
    std::string id;     /* datum keyword */
    std::string defn;   /* ie. "to_wgs84=..." */
    std::string ellipse_id; /* ie from ellipse table */
    std::string comments; /* EPSG code, etc */
};

struct PJ_PRIME_MERIDIANS
{
    std::string id;     /* prime meridian keyword */
    std::string defn;   /* offset from greenwich in DMS format. */
};

struct PJ_UNITS
{
    std::string id;    /* units keyword */
    std::string to_meter;    /* multiply by value to get meters */
    std::string name;    /* comments */
};

struct DERIVS
{
    double x_l, x_p; /* derivatives of x for lambda-phi */
    double y_l, y_p; /* derivatives of y for lambda-phi */
};

struct FACTORS
{
    struct DERIVS der;
    double h, k;    /* meridinal, parallel scales */
    double omega, thetap;    /* angular distortion, theta prime */
    double conv;    /* convergence */
    double s;        /* areal scale factor */
    double a, b;    /* max-min scale error */
    int code;        /* info as to analytics, see following */
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

/*!
    \brief parameters, projection parameters
    \details This structure initializes all projections
    \ingroup projection
*/
struct parameters : public detail::pj_const_pod
{
    std::string name;
    std::vector<detail::pvalue> params;
};

// TODO: derived from boost::exception / make more for forward/inverse/init/setup
class proj_exception
{
public:

    proj_exception(int code = 0)
        : m_code(code)
    {
    }
    int code() const { return m_code; }
private :
    int m_code;
};

}}} // namespace boost::geometry::projections
#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PROJECTS_HPP
