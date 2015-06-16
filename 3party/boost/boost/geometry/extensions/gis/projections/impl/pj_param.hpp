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

#ifndef BOOST_GEOMETRY_PROJECTIONS_PJ_PARAM_HPP
#define BOOST_GEOMETRY_PROJECTIONS_PJ_PARAM_HPP


#include <string>
#include <vector>

#include <boost/geometry/extensions/gis/geographic/strategies/dms_parser.hpp>

#include <boost/geometry/extensions/gis/projections/impl/projects.hpp>


namespace boost { namespace geometry { namespace projections {

namespace detail {



/* create pvalue list entry */
inline pvalue pj_mkparam(std::string const& str)
{
    std::string name = str;
    std::string value;
    boost::trim_left_if(name, boost::is_any_of("+"));
    std::string::size_type loc = name.find("=");
    if (loc != std::string::npos)
    {
        value = name.substr(loc + 1);
        name.erase(loc);
    }


    pvalue newitem;
    newitem.param = name;
    newitem.s = value;
    newitem.used = 0;
    newitem.i = atoi(value.c_str());
    newitem.f = atof(value.c_str());
    return newitem;
}

/************************************************************************/
/*                              pj_param()                              */
/*                                                                      */
/*      Test for presence or get pvalue value.  The first            */
/*      character in `opt' is a pvalue type which can take the       */
/*      values:                                                         */
/*                                                                      */
/*       `t' - test for presence, return TRUE/FALSE in pvalue.i         */
/*       `i' - integer value returned in pvalue.i                       */
/*       `d' - simple valued real input returned in pvalue.f            */
/*       `r' - degrees (DMS translation applied), returned as           */
/*             radians in pvalue.f                                      */
/*       `s' - string returned in pvalue.s                              */
/*       `b' - test for t/T/f/F, return in pvalue.i                     */
/*                                                                      */
/************************************************************************/

inline pvalue pj_param(std::vector<pvalue> const& pl, std::string opt)
{
    char type = opt[0];
    opt.erase(opt.begin());

    pvalue value;

    /* simple linear lookup */
    for (std::vector<pvalue>::const_iterator it = pl.begin(); it != pl.end(); it++)
    {
        if (it->param == opt)
        {
            //it->used = 1;
            switch (type)
            {
            case 't':
                value.i = 1;
                break;
            case 'i':    /* integer input */
                value.i = atoi(it->s.c_str());
                break;
            case 'd':    /* simple real input */
                value.f = atof(it->s.c_str());
                break;
            case 'r':    /* degrees input */
                {
                    geometry::strategy::dms_parser<true> parser;
                    value.f = parser(it->s.c_str());
                }
                break;
            case 's':    /* char string */
                value.s = it->s;
                break;
            case 'b':    /* boolean */
                switch (it->s[0])
                {
                case 'F': case 'f':
                    value.i = 0;
                    break;
                case '\0': case 'T': case 't':
                    value.i = 1;
                    break;
                default:
                    value.i = 0;
                    break;
                }
                break;
            }
            return value;
        }

    }

    value.i = 0;
    value.f = 0.0;
    value.s = "";
    return value;
}

} // namespace detail
}}} // namespace boost::geometry::projections

#endif
