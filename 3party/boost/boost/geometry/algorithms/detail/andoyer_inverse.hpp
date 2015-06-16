// Boost.Geometry

// Copyright (c) 2015 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ANDOYER_INVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ANDOYER_INVERSE_HPP


#include <boost/math/constants/constants.hpp>

#include <boost/geometry/core/radius.hpp>
#include <boost/geometry/core/srs.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/detail/flattening.hpp>


namespace boost { namespace geometry { namespace detail
{

/*!
\brief The solution of the inverse problem of geodesics on latlong coordinates,
       Forsyth-Andoyer-Lambert type approximation with first order terms.
\author See
    - Technical Report: PAUL D. THOMAS, MATHEMATICAL MODELS FOR NAVIGATION SYSTEMS, 1965
      http://www.dtic.mil/docs/citations/AD0627893
    - Technical Report: PAUL D. THOMAS, SPHEROIDAL GEODESICS, REFERENCE SYSTEMS, AND LOCAL GEOMETRY, 1970
      http://www.dtic.mil/docs/citations/AD703541
*/
template <typename CT>
class andoyer_inverse
{
public:
    template <typename T1, typename T2, typename Spheroid>
    andoyer_inverse(T1 const& lon1,
                    T1 const& lat1,
                    T2 const& lon2,
                    T2 const& lat2,
                    Spheroid const& spheroid)
        : m_a(get_radius<0>(spheroid))
        , m_b(get_radius<2>(spheroid))
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

        CT const pi_half = math::pi<CT>() / CT(2);

        if ( math::equals(math::abs(lat1), pi_half)
          && math::equals(math::abs(lat2), pi_half) )
        {
            m_is_result_zero = true;
            return;
        }

        CT const dlon = lon2 - lon1;
        m_sin_dlon = sin(dlon);
        m_cos_dlon = cos(dlon);
        m_sin_lat1 = sin(lat1);
        m_cos_lat1 = cos(lat1);
        m_sin_lat2 = sin(lat2);
        m_cos_lat2 = cos(lat2);

        // H,G,T = infinity if cos_d = 1 or cos_d = -1
        // lat1 == +-90 && lat2 == +-90
        // lat1 == lat2 && lon1 == lon2
        m_cos_d = m_sin_lat1*m_sin_lat2 + m_cos_lat1*m_cos_lat2*m_cos_dlon;
        m_d = acos(m_cos_d);
        m_sin_d = sin(m_d);

        // just in case since above lat1 and lat2 is checked
        // the check below is equal to cos_d == 1 || cos_d == -1 || d == 0
        if ( math::equals(m_sin_d, CT(0)) )
        {
            m_is_result_zero = true;
            return;
        }
    }

    inline CT distance() const
    {
        if ( m_is_result_zero )
        {
            // TODO return some approximated value
            return CT(0);
        }

        CT const K = math::sqr(m_sin_lat1-m_sin_lat2);
        CT const L = math::sqr(m_sin_lat1+m_sin_lat2);
        CT const three_sin_d = CT(3) * m_sin_d;
        // H or G = infinity if cos_d = 1 or cos_d = -1
        CT const H = (m_d+three_sin_d)/(CT(1)-m_cos_d);
        CT const G = (m_d-three_sin_d)/(CT(1)+m_cos_d);

        // for e.g. lat1=-90 && lat2=90 here we have G*L=INF*0
        CT const dd = -(m_f/CT(4))*(H*K+G*L);

        return m_a * (m_d + dd);
    }

    inline CT azimuth() const
    {
        // it's a situation when the endpoints are on the poles +-90 deg
        // in this case the azimuth could either be 0 or +-pi
        if ( m_is_result_zero )
        {
            return CT(0);
        }

        CT A = CT(0);
        CT U = CT(0);
        if ( ! math::equals(m_cos_lat2, CT(0)) )
        {
            CT const tan_lat2 = m_sin_lat2/m_cos_lat2;
            CT const M = m_cos_lat1*tan_lat2-m_sin_lat1*m_cos_dlon;
            A = atan2(m_sin_dlon, M);
            CT const sin_2A = sin(CT(2)*A);
            U = (m_f/CT(2))*math::sqr(m_cos_lat1)*sin_2A;
        }

        CT V = CT(0);
        if ( ! math::equals(m_cos_lat1, CT(0)) )
        {
            CT const tan_lat1 = m_sin_lat1/m_cos_lat1;
            CT const N = m_cos_lat2*tan_lat1-m_sin_lat2*m_cos_dlon;
            CT const B = atan2(m_sin_dlon, N);
            CT const sin_2B = sin(CT(2)*B);
            V = (m_f/CT(2))*math::sqr(m_cos_lat2)*sin_2B;
        }

        // infinity if sin_d = 0, so cos_d = 1 or cos_d = -1
        CT const T = m_d / m_sin_d;
        CT const dA = V*T-U;

        return A - dA;
    }

private:
    CT const m_a;
    CT const m_b;
    CT const m_f;

    CT m_sin_dlon;
    CT m_cos_dlon;
    CT m_sin_lat1;
    CT m_cos_lat1;
    CT m_sin_lat2;
    CT m_cos_lat2;

    CT m_cos_d;
    CT m_d;
    CT m_sin_d;

    bool m_is_result_zero;
};

}}} // namespace boost::geometry::detail


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ANDOYER_INVERSE_HPP
