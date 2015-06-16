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

#ifndef BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP
#define BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>

#include <boost/geometry/util/math.hpp>


#include <boost/geometry/extensions/gis/projections/impl/pj_datum_set.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_datums.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_ell_set.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_param.hpp>
#include <boost/geometry/extensions/gis/projections/impl/pj_units.hpp>
#include <boost/geometry/extensions/gis/projections/impl/projects.hpp>
#include <boost/geometry/extensions/gis/projections/parameters.hpp>

#include <boost/geometry/extensions/gis/geographic/strategies/dms_parser.hpp>


namespace boost { namespace geometry { namespace projections
{


namespace detail
{

/************************************************************************/
/*                              pj_init()                               */
/*                                                                      */
/*      Main entry point for initialing a PJ projections                */
/*      definition.  Note that the projection specific function is      */
/*      called to do the initial allocation so it can be created        */
/*      large enough to hold projection specific parameters.            */
/************************************************************************/
template <typename R>
inline parameters pj_init(R const& arguments, bool use_defaults = true)
{
    parameters pin;
    for (std::vector<std::string>::const_iterator it = boost::begin(arguments);
        it != boost::end(arguments); it++)
    {
        pin.params.push_back(pj_mkparam(*it));
    }

    /* check if +init present */
    if (pj_param(pin.params, "tinit").i)
    {
        // maybe TODO: handle "init" parameter
        //if (!(curr = get_init(&arguments, curr, pj_param(pin.params, "sinit").s)))
    }

    // find projection -> implemented in projection factory
    pin.name = pj_param(pin.params, "sproj").s;
    //if (pin.name.empty())
    //{ throw proj_exception(-4); }


    // set defaults, unless inhibited
    // GL-Addition, if use_defaults is false then defaults are ignored
    if (use_defaults && ! pj_param(pin.params, "bno_defs").i)
    {
        // proj4 gets defaults from "proj_def.dat", file of 94/02/23 with a few defaults.
        // Here manually
        if (pin.name == "lcc")
        {
            pin.params.push_back(pj_mkparam("lat_1=33"));
            pin.params.push_back(pj_mkparam("lat_2=45"));
        }
        else if (pin.name == "aea")
        {
            pin.params.push_back(pj_mkparam("lat_1=29.5"));
            pin.params.push_back(pj_mkparam("lat_2=45.5 "));
        }
        else
        {
            //<general>ellps=WGS84
        }
        //curr = get_defaults(&arguments, curr, name);
    }

    /* allocate projection structure */
    // done by constructor:
    // pin.is_latlong = 0;
    // pin.is_geocent = 0;
    // pin.long_wrap_center = 0.0;

    /* set datum parameters */
    pj_datum_set(pin.params, pin);

    /* set ellipsoid/sphere parameters */
    pj_ell_set(pin.params, pin.a, pin.es);

    pin.a_orig = pin.a;
    pin.es_orig = pin.es;

    pin.e = sqrt(pin.es);
    pin.ra = 1. / pin.a;
    pin.one_es = 1. - pin.es;
    if (pin.one_es == 0.) { throw proj_exception(-6); }
    pin.rone_es = 1./pin.one_es;

    /* Now that we have ellipse information check for WGS84 datum */
    if( pin.datum_type == PJD_3PARAM
        && pin.datum_params[0] == 0.0
        && pin.datum_params[1] == 0.0
        && pin.datum_params[2] == 0.0
        && pin.a == 6378137.0
        && geometry::math::abs(pin.es - 0.006694379990) < 0.000000000050 )/*WGS84/GRS80*/
    {
        pin.datum_type = PJD_WGS84;
    }

    /* set pin.geoc coordinate system */
    pin.geoc = (pin.es && pj_param(pin.params, "bgeoc").i);

    /* over-ranging flag */
    pin.over = pj_param(pin.params, "bover").i;

    /* longitude center for wrapping */
    pin.long_wrap_center = pj_param(pin.params, "rlon_wrap").f;

    /* central meridian */
    pin.lam0 = pj_param(pin.params, "rlon_0").f;

    /* central latitude */
    pin.phi0 = pj_param(pin.params, "rlat_0").f;

    /* false easting and northing */
    pin.x0 = pj_param(pin.params, "dx_0").f;
    pin.y0 = pj_param(pin.params, "dy_0").f;

    /* general scaling factor */
    if (pj_param(pin.params, "tk_0").i)
        pin.k0 = pj_param(pin.params, "dk_0").f;
    else if (pj_param(pin.params, "tk").i)
        pin.k0 = pj_param(pin.params, "dk").f;
    else
        pin.k0 = 1.;
    if (pin.k0 <= 0.) {
        throw proj_exception(-31);
    }

    /* set units */
    std::string s;
    std::string units = pj_param(pin.params, "sunits").s;
    if (! units.empty())
    {
        const int n = sizeof(pj_units) / sizeof(pj_units[0]);
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_units[i].id == units)
            {
                index = i;
            }
        }

        if (index == -1) { throw proj_exception(-7); }
        s = pj_units[index].to_meter;
    }

    if (s.empty())
    {
        s = pj_param(pin.params, "sto_meter").s;
    }

    if (! s.empty())
    {
        // TODO: IMPLEMENT SPLIT
        pin.to_meter = atof(s.c_str());
        //if (*s == '/') /* ratio number */
        //    pin.to_meter /= strtod(++s, 0);
        pin.fr_meter = 1. / pin.to_meter;
    }
    else
    {
        pin.to_meter = pin.fr_meter = 1.;
    }

    /* prime meridian */
    s.clear();
    std::string pm = pj_param(pin.params, "spm").s;
    if (! pm.empty())
    {
        std::string value;

        int n = sizeof(pj_prime_meridians) / sizeof(pj_prime_meridians[0]);
        int index = -1;
        for (int i = 0; i < n && index == -1; i++)
        {
            if(pj_prime_meridians[i].id == pm)
            {
                value = pj_prime_meridians[i].defn;
                index = i;
            }
        }

        if (index == -1) { throw proj_exception(-7); }
        if (value.empty()) { throw proj_exception(-46); }

        geometry::strategy::dms_parser<true> parser;
        pin.from_greenwich = parser(value.c_str());
    }
    else
    {
        pin.from_greenwich = 0.0;
    }

    return pin;
}

/************************************************************************/
/*                            pj_init_plus()                            */
/*                                                                      */
/*      Same as pj_init() except it takes one argument string with      */
/*      individual arguments preceeded by '+', such as "+proj=utm       */
/*      +zone=11 +ellps=WGS84".                                         */
/************************************************************************/

inline parameters pj_init_plus(std::string const& definition, bool use_defaults = true)
{
    const char* sep = " +";

    /* split into arguments based on '+' and trim white space */

    // boost::split splits on one character, here it should be on " +", so implementation below
    // todo: put in different routine or sort out
    std::vector<std::string> arguments;
    std::string def = boost::trim_copy(definition);
    boost::trim_left_if(def, boost::is_any_of(sep));

    std::string::size_type loc = def.find(sep);
    while (loc != std::string::npos)
    {
        std::string par = def.substr(0, loc);
        boost::trim(par);
        if (! par.empty())
        {
            arguments.push_back(par);
        }

        def.erase(0, loc);
        boost::trim_left_if(def, boost::is_any_of(sep));
        loc = def.find(sep);
    }

    if (! def.empty())
    {
        arguments.push_back(def);
    }

    /*boost::split(arguments, definition, boost::is_any_of("+"));
    for (std::vector<std::string>::iterator it = arguments.begin(); it != arguments.end(); it++)
    {
        boost::trim(*it);
    }*/
    return pj_init(arguments, use_defaults);
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif // BOOST_GEOMETRY_PROJECTIONS_IMPL_PJ_INIT_HPP
