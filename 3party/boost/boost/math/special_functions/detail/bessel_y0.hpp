//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_Y0_HPP
#define BOOST_MATH_BESSEL_Y0_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/detail/bessel_j0.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/rational.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/assert.hpp>

// Bessel function of the second kind of order zero
// x <= 8, minimax rational approximations on root-bracketing intervals
// x > 8, Hankel asymptotic expansion in Hart, Computer Approximations, 1968

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T bessel_y0(T x, const Policy& pol)
{
    static const T P1[] = {
         static_cast<T>(1.0723538782003176831e+11L),
        static_cast<T>(-8.3716255451260504098e+09L),
         static_cast<T>(2.0422274357376619816e+08L),
        static_cast<T>(-2.1287548474401797963e+06L),
         static_cast<T>(1.0102532948020907590e+04L),
        static_cast<T>(-1.8402381979244993524e+01L),
    };
    static const T Q1[] = {
         static_cast<T>(5.8873865738997033405e+11L),
         static_cast<T>(8.1617187777290363573e+09L),
         static_cast<T>(5.5662956624278251596e+07L),
         static_cast<T>(2.3889393209447253406e+05L),
         static_cast<T>(6.6475986689240190091e+02L),
         static_cast<T>(1.0L),
    };
    static const T P2[] = {
        static_cast<T>(-2.2213976967566192242e+13L),
        static_cast<T>(-5.5107435206722644429e+11L),
         static_cast<T>(4.3600098638603061642e+10L),
        static_cast<T>(-6.9590439394619619534e+08L),
         static_cast<T>(4.6905288611678631510e+06L),
        static_cast<T>(-1.4566865832663635920e+04L),
         static_cast<T>(1.7427031242901594547e+01L),
    };
    static const T Q2[] = {
         static_cast<T>(4.3386146580707264428e+14L),
         static_cast<T>(5.4266824419412347550e+12L),
         static_cast<T>(3.4015103849971240096e+10L),
         static_cast<T>(1.3960202770986831075e+08L),
         static_cast<T>(4.0669982352539552018e+05L),
         static_cast<T>(8.3030857612070288823e+02L),
         static_cast<T>(1.0L),
    };
    static const T P3[] = {
        static_cast<T>(-8.0728726905150210443e+15L),
         static_cast<T>(6.7016641869173237784e+14L),
        static_cast<T>(-1.2829912364088687306e+11L),
        static_cast<T>(-1.9363051266772083678e+11L),
         static_cast<T>(2.1958827170518100757e+09L),
        static_cast<T>(-1.0085539923498211426e+07L),
         static_cast<T>(2.1363534169313901632e+04L),
        static_cast<T>(-1.7439661319197499338e+01L),
    };
    static const T Q3[] = {
         static_cast<T>(3.4563724628846457519e+17L),
         static_cast<T>(3.9272425569640309819e+15L),
         static_cast<T>(2.2598377924042897629e+13L),
         static_cast<T>(8.6926121104209825246e+10L),
         static_cast<T>(2.4727219475672302327e+08L),
         static_cast<T>(5.3924739209768057030e+05L),
         static_cast<T>(8.7903362168128450017e+02L),
         static_cast<T>(1.0L),
    };
    static const T PC[] = {
         static_cast<T>(2.2779090197304684302e+04L),
         static_cast<T>(4.1345386639580765797e+04L),
         static_cast<T>(2.1170523380864944322e+04L),
         static_cast<T>(3.4806486443249270347e+03L),
         static_cast<T>(1.5376201909008354296e+02L),
         static_cast<T>(8.8961548424210455236e-01L),
    };
    static const T QC[] = {
         static_cast<T>(2.2779090197304684318e+04L),
         static_cast<T>(4.1370412495510416640e+04L),
         static_cast<T>(2.1215350561880115730e+04L),
         static_cast<T>(3.5028735138235608207e+03L),
         static_cast<T>(1.5711159858080893649e+02L),
         static_cast<T>(1.0L),
    };
    static const T PS[] = {
        static_cast<T>(-8.9226600200800094098e+01L),
        static_cast<T>(-1.8591953644342993800e+02L),
        static_cast<T>(-1.1183429920482737611e+02L),
        static_cast<T>(-2.2300261666214198472e+01L),
        static_cast<T>(-1.2441026745835638459e+00L),
        static_cast<T>(-8.8033303048680751817e-03L),
    };
    static const T QS[] = {
         static_cast<T>(5.7105024128512061905e+03L),
         static_cast<T>(1.1951131543434613647e+04L),
         static_cast<T>(7.2642780169211018836e+03L),
         static_cast<T>(1.4887231232283756582e+03L),
         static_cast<T>(9.0593769594993125859e+01L),
         static_cast<T>(1.0L),
    };
    static const T x1  =  static_cast<T>(8.9357696627916752158e-01L),
                   x2  =  static_cast<T>(3.9576784193148578684e+00L),
                   x3  =  static_cast<T>(7.0860510603017726976e+00L),
                   x11 =  static_cast<T>(2.280e+02L),
                   x12 =  static_cast<T>(2.9519662791675215849e-03L),
                   x21 =  static_cast<T>(1.0130e+03L),
                   x22 =  static_cast<T>(6.4716931485786837568e-04L),
                   x31 =  static_cast<T>(1.8140e+03L),
                   x32 =  static_cast<T>(1.1356030177269762362e-04L)
    ;
    T value, factor, r, rc, rs;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;
    using namespace boost::math::constants;

    static const char* function = "boost::math::bessel_y0<%1%>(%1%,%1%)";

    if (x < 0)
    {
       return policies::raise_domain_error<T>(function,
            "Got x = %1% but x must be non-negative, complex result not supported.", x, pol);
    }
    if (x == 0)
    {
       return -policies::raise_overflow_error<T>(function, 0, pol);
    }
    if (x <= 3)                       // x in (0, 3]
    {
        T y = x * x;
        T z = 2 * log(x/x1) * bessel_j0(x) / pi<T>();
        r = evaluate_rational(P1, Q1, y);
        factor = (x + x1) * ((x - x11/256) - x12);
        value = z + factor * r;
    }
    else if (x <= 5.5f)                  // x in (3, 5.5]
    {
        T y = x * x;
        T z = 2 * log(x/x2) * bessel_j0(x) / pi<T>();
        r = evaluate_rational(P2, Q2, y);
        factor = (x + x2) * ((x - x21/256) - x22);
        value = z + factor * r;
    }
    else if (x <= 8)                  // x in (5.5, 8]
    {
        T y = x * x;
        T z = 2 * log(x/x3) * bessel_j0(x) / pi<T>();
        r = evaluate_rational(P3, Q3, y);
        factor = (x + x3) * ((x - x31/256) - x32);
        value = z + factor * r;
    }
    else                                // x in (8, \infty)
    {
        T y = 8 / x;
        T y2 = y * y;
        T z = x - 0.25f * pi<T>();
        rc = evaluate_rational(PC, QC, y2);
        rs = evaluate_rational(PS, QS, y2);
        factor = sqrt(2 / (x * pi<T>()));
        value = factor * (rc * sin(z) + y * rs * cos(z));
    }

    return value;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_Y0_HPP

