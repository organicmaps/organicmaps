//  Copyright (c) 2006 Xiaogang Zhang
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_K0_HPP
#define BOOST_MATH_BESSEL_K0_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/rational.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/assert.hpp>

// Modified Bessel function of the second kind of order zero
// minimax rational approximations on intervals, see
// Russon and Blair, Chalk River Report AECL-3461, 1969

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T bessel_k0(T x, const Policy& pol)
{
    BOOST_MATH_INSTRUMENT_CODE(x);

    static const T P1[] = {
         static_cast<T>(2.4708152720399552679e+03L),
         static_cast<T>(5.9169059852270512312e+03L),
         static_cast<T>(4.6850901201934832188e+02L),
         static_cast<T>(1.1999463724910714109e+01L),
         static_cast<T>(1.3166052564989571850e-01L),
         static_cast<T>(5.8599221412826100000e-04L)
    };
    static const T Q1[] = {
         static_cast<T>(2.1312714303849120380e+04L),
        static_cast<T>(-2.4994418972832303646e+02L),
         static_cast<T>(1.0L)
    };
    static const T P2[] = {
        static_cast<T>(-1.6128136304458193998e+06L),
        static_cast<T>(-3.7333769444840079748e+05L),
        static_cast<T>(-1.7984434409411765813e+04L),
        static_cast<T>(-2.9501657892958843865e+02L),
        static_cast<T>(-1.6414452837299064100e+00L)
    };
    static const T Q2[] = {
        static_cast<T>(-1.6128136304458193998e+06L),
        static_cast<T>(2.9865713163054025489e+04L),
        static_cast<T>(-2.5064972445877992730e+02L),
        static_cast<T>(1.0L)
    };
    static const T P3[] = {
         static_cast<T>(1.1600249425076035558e+02L),
         static_cast<T>(2.3444738764199315021e+03L),
         static_cast<T>(1.8321525870183537725e+04L),
         static_cast<T>(7.1557062783764037541e+04L),
         static_cast<T>(1.5097646353289914539e+05L),
         static_cast<T>(1.7398867902565686251e+05L),
         static_cast<T>(1.0577068948034021957e+05L),
         static_cast<T>(3.1075408980684392399e+04L),
         static_cast<T>(3.6832589957340267940e+03L),
         static_cast<T>(1.1394980557384778174e+02L)
    };
    static const T Q3[] = {
         static_cast<T>(9.2556599177304839811e+01L),
         static_cast<T>(1.8821890840982713696e+03L),
         static_cast<T>(1.4847228371802360957e+04L),
         static_cast<T>(5.8824616785857027752e+04L),
         static_cast<T>(1.2689839587977598727e+05L),
         static_cast<T>(1.5144644673520157801e+05L),
         static_cast<T>(9.7418829762268075784e+04L),
         static_cast<T>(3.1474655750295278825e+04L),
         static_cast<T>(4.4329628889746408858e+03L),
         static_cast<T>(2.0013443064949242491e+02L),
         static_cast<T>(1.0L)
    };
    T value, factor, r, r1, r2;

    BOOST_MATH_STD_USING
    using namespace boost::math::tools;

    static const char* function = "boost::math::bessel_k0<%1%>(%1%,%1%)";

    if (x < 0)
    {
       return policies::raise_domain_error<T>(function,
            "Got x = %1%, but argument x must be non-negative, complex number result not supported", x, pol);
    }
    if (x == 0)
    {
       return policies::raise_overflow_error<T>(function, 0, pol);
    }
    if (x <= 1)                         // x in (0, 1]
    {
        T y = x * x;
        r1 = evaluate_polynomial(P1, y) / evaluate_polynomial(Q1, y);
        r2 = evaluate_polynomial(P2, y) / evaluate_polynomial(Q2, y);
        factor = log(x);
        value = r1 - factor * r2;
    }
    else                                // x in (1, \infty)
    {
        T y = 1 / x;
        r = evaluate_polynomial(P3, y) / evaluate_polynomial(Q3, y);
        factor = exp(-x) / sqrt(x);
        value = factor * r;
        BOOST_MATH_INSTRUMENT_CODE("y = " << y);
        BOOST_MATH_INSTRUMENT_CODE("r = " << r);
        BOOST_MATH_INSTRUMENT_CODE("factor = " << factor);
        BOOST_MATH_INSTRUMENT_CODE("value = " << value);
    }

    return value;
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_K0_HPP

