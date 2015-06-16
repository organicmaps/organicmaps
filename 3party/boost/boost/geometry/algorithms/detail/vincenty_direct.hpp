// Boost.Geometry

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_VINCENTY_DIRECT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_VINCENTY_DIRECT_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/srs.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/detail/flattening.hpp>


#ifndef BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS
#define BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS 1000
#endif


namespace boost { namespace geometry { namespace detail
{

/*!
\brief The solution of the direct problem of geodesics on latlong coordinates, after Vincenty, 1975
\author See
    - http://www.ngs.noaa.gov/PUBS_LIB/inverse.pdf
    - http://www.icsm.gov.au/gda/gdav2.3.pdf
\author Adapted from various implementations to get it close to the original document
    - http://www.movable-type.co.uk/scripts/LatLongVincenty.html
    - http://exogen.case.edu/projects/geopy/source/geopy.distance.html
    - http://futureboy.homeip.net/fsp/colorize.fsp?fileName=navigation.frink

*/
template <typename CT>
class vincenty_direct
{
public:
    template <typename T, typename Dist, typename Azi, typename Spheroid>
    vincenty_direct(T const& lo1,
                    T const& la1,
                    Dist const& distance,
                    Azi const& azimuth12,
                    Spheroid const& spheroid)
        : lon1(lo1)
        , lat1(la1)
        , is_distance_zero(false)
    {
        if ( math::equals(distance, Dist(0)) || distance < Dist(0) )
        {
            is_distance_zero = true;
            return;
        }

        CT const radius_a = CT(get_radius<0>(spheroid));
        CT const radius_b = CT(get_radius<2>(spheroid));
        flattening = geometry::detail::flattening<CT>(spheroid);

        sin_azimuth12 = sin(azimuth12);
        cos_azimuth12 = cos(azimuth12);

        // U: reduced latitude, defined by tan U = (1-f) tan phi
        one_min_f = CT(1) - flattening;
        CT const tan_U1 = one_min_f * tan(lat1);
        CT const sigma1 = atan2(tan_U1, cos_azimuth12); // (1)

        // may be calculated from tan using 1 sqrt()
        CT const U1 = atan(tan_U1);
        sin_U1 = sin(U1);
        cos_U1 = cos(U1);

        sin_alpha = cos_U1 * sin_azimuth12; // (2)
        sin_alpha_sqr = math::sqr(sin_alpha);
        cos_alpha_sqr = CT(1) - sin_alpha_sqr;

        CT const b_sqr = radius_b * radius_b;
        CT const u_sqr = cos_alpha_sqr * (radius_a * radius_a - b_sqr) / b_sqr;
        CT const A = CT(1) + (u_sqr/CT(16384)) * (CT(4096) + u_sqr*(CT(-768) + u_sqr*(CT(320) - u_sqr*CT(175)))); // (3)
        CT const B = (u_sqr/CT(1024))*(CT(256) + u_sqr*(CT(-128) + u_sqr*(CT(74) - u_sqr*CT(47)))); // (4)

        CT s_div_bA = distance / (radius_b * A);
        sigma = s_div_bA; // (7)

        CT previous_sigma;

        int counter = 0; // robustness

        do
        {
            previous_sigma = sigma;

            CT const two_sigma_m = CT(2) * sigma1 + sigma; // (5)

            sin_sigma = sin(sigma);
            cos_sigma = cos(sigma);
            CT const sin_sigma_sqr = math::sqr(sin_sigma);
            cos_2sigma_m = cos(two_sigma_m);
            cos_2sigma_m_sqr = math::sqr(cos_2sigma_m);

            CT const delta_sigma = B * sin_sigma * (cos_2sigma_m
                                        + (B/CT(4)) * ( cos_sigma * (CT(-1) + CT(2)*cos_2sigma_m_sqr)
                                            - (B/CT(6) * cos_2sigma_m * (CT(-3)+CT(4)*sin_sigma_sqr) * (CT(-3)+CT(4)*cos_2sigma_m_sqr)) )); // (6)

            sigma = s_div_bA + delta_sigma; // (7)

            ++counter; // robustness

        } while ( geometry::math::abs(previous_sigma - sigma) > CT(1e-12)
               //&& geometry::math::abs(sigma) < pi
               && counter < BOOST_GEOMETRY_DETAIL_VINCENTY_MAX_STEPS ); // robustness
    }

    inline CT lat2() const
    {
        if ( is_distance_zero )
        {
            return lat1;
        }

        return atan2( sin_U1 * cos_sigma + cos_U1 * sin_sigma * cos_azimuth12,
                      one_min_f * math::sqrt(sin_alpha_sqr + math::sqr(sin_U1 * sin_sigma - cos_U1 * cos_sigma * cos_azimuth12))); // (8)
    }

    inline CT lon2() const
    {
        if ( is_distance_zero )
        {
            return lon1;
        }

        CT const lambda = atan2( sin_sigma * sin_azimuth12,
                                 cos_U1 * cos_sigma - sin_U1 * sin_sigma * cos_azimuth12); // (9)
        CT const C = (flattening/CT(16)) * cos_alpha_sqr * ( CT(4) + flattening * ( CT(4) - CT(3) * cos_alpha_sqr ) ); // (10)
        CT const L = lambda - (CT(1) - C) * flattening * sin_alpha
                        * ( sigma + C * sin_sigma * ( cos_2sigma_m + C * cos_sigma * ( CT(-1) + CT(2) * cos_2sigma_m_sqr ) ) ); // (11)

        return lon1 + L;
    }

    inline CT azimuth21() const
    {
        // NOTE: signs of X and Y are different than in the original paper
        return is_distance_zero ?
               CT(0) :
               atan2(-sin_alpha, sin_U1 * sin_sigma - cos_U1 * cos_sigma * cos_azimuth12); // (12)
    }

private:
    CT sigma;
    CT sin_sigma;
    CT cos_sigma;

    CT cos_2sigma_m;
    CT cos_2sigma_m_sqr;

    CT sin_alpha;
    CT sin_alpha_sqr;
    CT cos_alpha_sqr;

    CT sin_azimuth12;
    CT cos_azimuth12;

    CT sin_U1;
    CT cos_U1;

    CT flattening;
    CT one_min_f;

    CT const lon1;
    CT const lat1;

    bool is_distance_zero;
};

}}} // namespace boost::geometry::detail


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_VINCENTY_DIRECT_HPP
