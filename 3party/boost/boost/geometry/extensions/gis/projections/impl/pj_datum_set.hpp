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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <boost/geometry/extensions/gis/projections/impl/pj_datums.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_param.hpp>
#include <boost/geometry/extensions/gis/projections/parameters.hpp>

namespace boost { namespace geometry { namespace projections {

namespace detail {


/* SEC_TO_RAD = Pi/180/3600 */
const double SEC_TO_RAD = 4.84813681109535993589914102357e-6;

/************************************************************************/
/*                            pj_datum_set()                            */
/************************************************************************/

inline void pj_datum_set(std::vector<pvalue>& pvalues, parameters& projdef)
{
    std::string name, towgs84, nadgrids;

    projdef.datum_type = PJD_UNKNOWN;

    /* -------------------------------------------------------------------- */
    /*      Is there a datum definition in the parameter list?  If so,     */
    /*      add the defining values to the parameter list.  Note that       */
    /*      this will append the ellipse definition as well as the          */
    /*      towgs84= and related parameters.  It should also be pointed     */
    /*      out that the addition is permanent rather than temporary        */
    /*      like most other keyword expansion so that the ellipse           */
    /*      definition will last into the pj_ell_set() function called      */
    /*      after this one.                                                 */
    /* -------------------------------------------------------------------- */
    name = pj_param(pvalues, "sdatum").s;
    if(! name.empty())
    {
        /* find the datum definition */
        const int n = sizeof(pj_datums) / sizeof(pj_datums[0]);
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_datums[i].id == name)
            {
                index = i;
            }
        }

        if (index == -1)
        {
            throw proj_exception(-9);
        }

        if(! pj_datums[index].ellipse_id.empty())
        {
            std::string entry("ellps=");
            entry +=pj_datums[index].ellipse_id;
            pvalues.push_back(pj_mkparam(entry));
        }

        if(! pj_datums[index].defn.empty())
        {
            pvalues.push_back(pj_mkparam(pj_datums[index].defn));
        }
    }

/* -------------------------------------------------------------------- */
/*      Check for nadgrids parameter.                                   */
/* -------------------------------------------------------------------- */
    nadgrids = pj_param(pvalues, "snadgrids").s;
    towgs84 = pj_param(pvalues, "stowgs84").s;
    if(! nadgrids.empty())
    {
        /* We don't actually save the value separately.  It will continue
           to exist int he param list for use in pj_apply_gridshift.c */

        projdef.datum_type = PJD_GRIDSHIFT;
    }

/* -------------------------------------------------------------------- */
/*      Check for towgs84 parameter.                                    */
/* -------------------------------------------------------------------- */
    else if(! towgs84.empty())
    {
        int parm_count = 0;

        int n = sizeof(projdef.datum_params) / sizeof(projdef.datum_params[0]);

        /* parse out the pvalues */
        std::vector<std::string> parm;
        boost::split(parm, towgs84, boost::is_any_of(" ,"));
        for (std::vector<std::string>::const_iterator it = parm.begin();
            it != parm.end() && parm_count < n;
            ++it)
        {
            projdef.datum_params[parm_count++] = atof(it->c_str());
        }

        if( projdef.datum_params[3] != 0.0
            || projdef.datum_params[4] != 0.0
            || projdef.datum_params[5] != 0.0
            || projdef.datum_params[6] != 0.0 )
        {
            projdef.datum_type = PJD_7PARAM;

            /* transform from arc seconds to radians */
            projdef.datum_params[3] *= SEC_TO_RAD;
            projdef.datum_params[4] *= SEC_TO_RAD;
            projdef.datum_params[5] *= SEC_TO_RAD;
            /* transform from parts per million to scaling factor */
            projdef.datum_params[6] =
                (projdef.datum_params[6]/1000000.0) + 1;
        }
        else
        {
            projdef.datum_type = PJD_3PARAM;
        }

        /* Note that pj_init() will later switch datum_type to
           PJD_WGS84 if shifts are all zero, and ellipsoid is WGS84 or GRS80 */
    }
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_DATUM_SET_HPP
