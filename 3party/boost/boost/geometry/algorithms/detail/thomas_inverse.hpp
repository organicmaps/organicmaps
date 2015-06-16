// Boost.Geometry

// Copyright (c) 2015 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_THOMAS_INVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_THOMAS_INVERSE_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/srs.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/detail/flattening.hpp>


namespace boost { namespace geometry { namespace detail
{

/*!
\brief The solution of the inverse problem of geodesics on latlong coordinates,
       Forsyth-Andoyer-Lambert type approximation with second order terms.
\author See
    - Technical Report: PAUL D. THOMAS, MATHEMATICAL MODELS FOR NAVIGATION SYSTEMS, 1965
      http://www.dtic.mil/docs/citations/AD0627893
    - Technical Report: PAUL D. THOMAS, SPHEROIDAL GEODESICS, REFERENCE SYSTEMS, AND LOCAL GEOMETRY, 1970
      http://www.dtic.mil/docs/citations/AD703541
*/
template <typename CT>
class thomas_inverse
{
public:
    template <typename T1, typename T2, typename Spheroid>
    thomas_inverse(T1 const& lon1,
                   T1 const& lat1,
                   T2 const& lon2,
                   T2 const& lat2,
                   Spheroid const& spheroid)
        : m_a(get_radius<0>(spheroid))
        , m_f(detail::flattening<CT>(spheroid))
        , m_is_result_zero(false)
    {
        // coordinates in radians

        if ( math::equals(lon1, lon2)
          && math::equals(lat1, lat2) )
        {
            m_is_result_zero = true;
            return;
        }

        CT const one_minus_f = CT(1) - m_f;

//        CT const tan_theta1 = one_minus_f * tan(lat1);
//        CT const tan_theta2 = one_minus_f * tan(lat2);
//        CT const theta1 = atan(tan_theta1);
//        CT const theta2 = atan(tan_theta2);

        CT const pi_half = math::pi<CT>() / CT(2);
        CT const theta1 = math::equals(lat1, pi_half) ? lat1 :
                          math::equals(lat1, -pi_half) ? lat1 :
                          atan(one_minus_f * tan(lat1));
        CT const theta2 = math::equals(lat2, pi_half) ? lat2 :
                          math::equals(lat2, -pi_half) ? lat2 :
                          atan(one_minus_f * tan(lat2));

        CT const theta_m = (theta1 + theta2) / CT(2);
        CT const d_theta_m = (theta2 - theta1) / CT(2);
        m_d_lambda = lon2 - lon1;
        CT const d_lambda_m = m_d_lambda / CT(2);

        m_sin_theta_m = sin(theta_m);
        m_cos_theta_m = cos(theta_m);
        m_sin_d_theta_m = sin(d_theta_m);
        m_cos_d_theta_m = cos(d_theta_m);
        CT const sin2_theta_m = math::sqr(m_sin_theta_m);
        CT const cos2_theta_m = math::sqr(m_cos_theta_m);
        CT const sin2_d_theta_m = math::sqr(m_sin_d_theta_m);
        CT const cos2_d_theta_m = math::sqr(m_cos_d_theta_m);
        CT const sin_d_lambda_m = sin(d_lambda_m);
        CT const sin2_d_lambda_m = math::sqr(sin_d_lambda_m);

        CT const H = cos2_theta_m - sin2_d_theta_m;
        CT const L = sin2_d_theta_m + H * sin2_d_lambda_m;
        m_cos_d = CT(1) - CT(2) * L;
        CT const d = acos(m_cos_d);
        m_sin_d = sin(d);

        CT const one_minus_L = CT(1) - L;

        if ( math::equals(m_sin_d, CT(0))
          || math::equals(L, CT(0))
          || math::equals(one_minus_L, CT(0)) )
        {
            m_is_result_zero = true;
            return;
        }

        CT const U = CT(2) * sin2_theta_m * cos2_d_theta_m / one_minus_L;
        CT const V = CT(2) * sin2_d_theta_m * cos2_theta_m / L;
        m_X = U + V;
        m_Y = U - V;
        m_T = d / m_sin_d;
        //CT const D = CT(4) * math::sqr(T);
        //CT const E = CT(2) * cos_d;
        //CT const A = D * E;
        //CT const B = CT(2) * D;
        //CT const C = T - (A - E) / CT(2);
    }

    inline CT distance() const
    {
        if ( m_is_result_zero )
        {
            // TODO return some approximated value
            return CT(0);
        }

        //CT const n1 = X * (A + C*X);
        //CT const n2 = Y * (B + E*Y);
        //CT const n3 = D*X*Y;

        //CT const f_sqr = math::sqr(f);
        //CT const f_sqr_per_64 = f_sqr / CT(64);

        CT const delta1d = m_f * (m_T*m_X-m_Y) / CT(4);
        //CT const delta2d = f_sqr_per_64 * (n1 - n2 + n3);

        return m_a * m_sin_d * (m_T - delta1d);
        //double S2 = a * sin_d * (T - delta1d + delta2d);
    }

    inline CT azimuth() const
    {
        // NOTE: if both cos_latX == 0 then below we'd have 0 * INF
        // it's a situation when the endpoints are on the poles +-90 deg
        // in this case the azimuth could either be 0 or +-pi
        if ( m_is_result_zero )
        {
            return CT(0);
        }

        // may also be used to calculate distance21
        //CT const D = CT(4) * math::sqr(T);
        CT const E = CT(2) * m_cos_d;
        //CT const A = D * E;
        //CT const B = CT(2) * D;
        // may also be used to calculate distance21
        CT const f_sqr = math::sqr(m_f);
        CT const f_sqr_per_64 = f_sqr / CT(64);

        CT const F = CT(2)*m_Y-E*(CT(4)-m_X);
        //CT const M = CT(32)*T-(CT(20)*T-A)*X-(B+CT(4))*Y;
        CT const G = m_f*m_T/CT(2) + f_sqr_per_64;
        CT const tan_d_lambda = tan(m_d_lambda);
        CT const Q = -(F*G*tan_d_lambda) / CT(4);

        CT const d_lambda_p = (m_d_lambda + Q) / CT(2);
        CT const tan_d_lambda_p = tan(d_lambda_p);

        CT const v = atan2(m_cos_d_theta_m, m_sin_theta_m * tan_d_lambda_p);
        CT const u = atan2(-m_sin_d_theta_m, m_cos_theta_m * tan_d_lambda_p);

        CT const pi = math::pi<CT>();
        CT alpha1 = v + u;
        if ( alpha1 > pi )
        {
            alpha1 -= CT(2) * pi;
        }

        return alpha1;
    }

private:
    CT const m_a;
    CT const m_f;

    CT m_d_lambda;
    CT m_cos_d;
    CT m_sin_d;
    CT m_X;
    CT m_Y;
    CT m_T;
    CT m_sin_theta_m;
    CT m_cos_theta_m;
    CT m_sin_d_theta_m;
    CT m_cos_d_theta_m;

    bool m_is_result_zero;
};

}}} // namespace boost::geometry::detail


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_THOMAS_INVERSE_HPP
