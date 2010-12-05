//  Copyright (c) 2007 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This is a partial header, do not include on it's own!!!
//
// Contains asymptotic expansions for Bessel J(v,x) and Y(v,x)
// functions, as x -> INF.
//
#ifndef BOOST_MATH_SF_DETAIL_BESSEL_JY_ASYM_HPP
#define BOOST_MATH_SF_DETAIL_BESSEL_JY_ASYM_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/factorials.hpp>

namespace boost{ namespace math{ namespace detail{

template <class T>
inline T asymptotic_bessel_j_large_x_P(T v, T x)
{
   // A&S 9.2.9
   T s = 1;
   T mu = 4 * v * v;
   T ez2 = 8 * x;
   ez2 *= ez2;
   s -= (mu-1) * (mu-9) / (2 * ez2);
   s += (mu-1) * (mu-9) * (mu-25) * (mu - 49) / (24 * ez2 * ez2);
   return s;
}

template <class T>
inline T asymptotic_bessel_j_large_x_Q(T v, T x)
{
   // A&S 9.2.10
   T s = 0;
   T mu = 4 * v * v;
   T ez = 8*x;
   s += (mu-1) / ez;
   s -= (mu-1) * (mu-9) * (mu-25) / (6 * ez*ez*ez);
   return s;
}

template <class T>
inline T asymptotic_bessel_j_large_x(T v, T x)
{
   // 
   // See http://functions.wolfram.com/BesselAiryStruveFunctions/BesselJ/06/02/02/0001/
   //
   // Also A&S 9.2.5
   //
   BOOST_MATH_STD_USING // ADL of std names
   T chi = fabs(x) - constants::pi<T>() * (2 * v + 1) / 4;
   return sqrt(2 / (constants::pi<T>() * x))
      * (asymptotic_bessel_j_large_x_P(v, x) * cos(chi) 
         - asymptotic_bessel_j_large_x_Q(v, x) * sin(chi));
}

template <class T>
inline T asymptotic_bessel_y_large_x(T v, T x)
{
   // 
   // See http://functions.wolfram.com/BesselAiryStruveFunctions/BesselJ/06/02/02/0001/
   //
   // Also A&S 9.2.5
   //
   BOOST_MATH_STD_USING // ADL of std names
   T chi = fabs(x) - constants::pi<T>() * (2 * v + 1) / 4;
   return sqrt(2 / (constants::pi<T>() * x))
      * (asymptotic_bessel_j_large_x_P(v, x) * sin(chi) 
         - asymptotic_bessel_j_large_x_Q(v, x) * cos(chi));
}

template <class T>
inline T asymptotic_bessel_amplitude(T v, T x)
{
   // Calculate the amplitude of J(v, x) and Y(v, x) for large
   // x: see A&S 9.2.28.
   BOOST_MATH_STD_USING
   T s = 1;
   T mu = 4 * v * v;
   T txq = 2 * x;
   txq *= txq;

   s += (mu - 1) / (2 * txq);
   s += 3 * (mu - 1) * (mu - 9) / (txq * txq * 8);
   s += 15 * (mu - 1) * (mu - 9) * (mu - 25) / (txq * txq * txq * 8 * 6);

   return sqrt(s * 2 / (constants::pi<T>() * x));
}

template <class T>
T asymptotic_bessel_phase_mx(T v, T x)
{
   //
   // Calculate the phase of J(v, x) and Y(v, x) for large x.
   // See A&S 9.2.29.
   // Note that the result returned is the phase less x.
   //
   T mu = 4 * v * v;
   T denom = 4 * x;
   T denom_mult = denom * denom;

   T s = -constants::pi<T>() * (v / 2 + 0.25f);
   s += (mu - 1) / (2 * denom);
   denom *= denom_mult;
   s += (mu - 1) * (mu - 25) / (6 * denom);
   denom *= denom_mult;
   s += (mu - 1) * (mu * mu - 114 * mu + 1073) / (5 * denom);
   denom *= denom_mult;
   s += (mu - 1) * (5 * mu * mu * mu - 1535 * mu * mu + 54703 * mu - 375733) / (14 * denom);
   return s;
}

template <class T>
inline T asymptotic_bessel_y_large_x_2(T v, T x)
{
   // See A&S 9.2.19.
   BOOST_MATH_STD_USING
   // Get the phase and amplitude:
   T ampl = asymptotic_bessel_amplitude(v, x);
   T phase = asymptotic_bessel_phase_mx(v, x);
   //
   // Calculate the sine of the phase, using:
   // sin(x+p) = sin(x)cos(p) + cos(x)sin(p)
   //
   T sin_phase = sin(phase) * cos(x) + cos(phase) * sin(x);
   return sin_phase * ampl;
}

template <class T>
inline T asymptotic_bessel_j_large_x_2(T v, T x)
{
   // See A&S 9.2.19.
   BOOST_MATH_STD_USING
   // Get the phase and amplitude:
   T ampl = asymptotic_bessel_amplitude(v, x);
   T phase = asymptotic_bessel_phase_mx(v, x);
   //
   // Calculate the sine of the phase, using:
   // cos(x+p) = cos(x)cos(p) - sin(x)sin(p)
   //
   T sin_phase = cos(phase) * cos(x) - sin(phase) * sin(x);
   return sin_phase * ampl;
}

//
// Various limits for the J and Y asymptotics
// (the asympotic expansions are safe to use if
// x is less than the limit given).
// We assume that if we don't use these expansions then the
// error will likely be >100eps, so the limits given are chosen
// to lead to < 100eps truncation error.
//
template <class T>
inline T asymptotic_bessel_y_limit(const mpl::int_<0>&)
{
   // default case:
   BOOST_MATH_STD_USING
   return 2.25 / pow(100 * tools::epsilon<T>() / T(0.001f), T(0.2f));
}
template <class T>
inline T asymptotic_bessel_y_limit(const mpl::int_<53>&)
{
   // double case:
   return 304 /*780*/;
}
template <class T>
inline T asymptotic_bessel_y_limit(const mpl::int_<64>&)
{
   // 80-bit extended-double case:
   return 1552 /*3500*/;
}
template <class T>
inline T asymptotic_bessel_y_limit(const mpl::int_<113>&)
{
   // 128-bit long double case:
   return 1245243 /*3128000*/;
}

template <class T, class Policy>
struct bessel_asymptotic_tag
{
   typedef typename policies::precision<T, Policy>::type precision_type;
   typedef typename mpl::if_<
      mpl::or_<
         mpl::equal_to<precision_type, mpl::int_<0> >,
         mpl::greater<precision_type, mpl::int_<113> > >,
      mpl::int_<0>,
      typename mpl::if_<
         mpl::greater<precision_type, mpl::int_<64> >,
         mpl::int_<113>,
         typename mpl::if_<
            mpl::greater<precision_type, mpl::int_<53> >,
            mpl::int_<64>,
            mpl::int_<53>
         >::type
      >::type
   >::type type;
};

template <class T>
inline T asymptotic_bessel_j_limit(const T& v, const mpl::int_<0>&)
{
   // default case:
   BOOST_MATH_STD_USING
   T v2 = (std::max)(T(3), T(v * v));
   return v2 / pow(100 * tools::epsilon<T>() / T(2e-5f), T(0.17f));
}
template <class T>
inline T asymptotic_bessel_j_limit(const T& v, const mpl::int_<53>&)
{
   // double case:
   T v2 = (std::max)(T(3), v * v);
   return v2 * 33 /*73*/;
}
template <class T>
inline T asymptotic_bessel_j_limit(const T& v, const mpl::int_<64>&)
{
   // 80-bit extended-double case:
   T v2 = (std::max)(T(3), v * v);
   return v2 * 121 /*266*/;
}
template <class T>
inline T asymptotic_bessel_j_limit(const T& v, const mpl::int_<113>&)
{
   // 128-bit long double case:
   T v2 = (std::max)(T(3), v * v);
   return v2 * 39154 /*85700*/;
}

template <class T, class Policy>
void temme_asyptotic_y_small_x(T v, T x, T* Y, T* Y1, const Policy& pol)
{
   T c = 1;
   T p = (v / boost::math::sin_pi(v, pol)) * pow(x / 2, -v) / boost::math::tgamma(1 - v, pol);
   T q = (v / boost::math::sin_pi(v, pol)) * pow(x / 2, v) / boost::math::tgamma(1 + v, pol);
   T f = (p - q) / v;
   T g_prefix = boost::math::sin_pi(v / 2, pol);
   g_prefix *= g_prefix * 2 / v;
   T g = f + g_prefix * q;
   T h = p;
   T c_mult = -x * x / 4;

   T y(c * g), y1(c * h);

   for(int k = 1; k < policies::get_max_series_iterations<Policy>(); ++k)
   {
      f = (k * f + p + q) / (k*k - v*v);
      p /= k - v;
      q /= k + v;
      c *= c_mult / k;
      T c1 = pow(-x * x / 4, k) / factorial<T>(k, pol);
      g = f + g_prefix * q;
      h = -k * g + p;
      y += c * g;
      y1 += c * h;
      if(c * g / tools::epsilon<T>() < y)
         break;
   }

   *Y = -y;
   *Y1 = (-2 / x) * y1;
}

template <class T, class Policy>
T asymptotic_bessel_i_large_x(T v, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING  // ADL of std names
   T s = 1;
   T mu = 4 * v * v;
   T ex = 8 * x;
   T num = mu - 1;
   T denom = ex;

   s -= num / denom;

   num *= mu - 9;
   denom *= ex * 2;
   s += num / denom;

   num *= mu - 25;
   denom *= ex * 3;
   s -= num / denom;

   // Try and avoid overflow to the last minute:
   T e = exp(x/2);

   s = e * (e * s / sqrt(2 * x * constants::pi<T>()));

   return (boost::math::isfinite)(s) ? 
      s : policies::raise_overflow_error<T>("boost::math::asymptotic_bessel_i_large_x<%1%>(%1%,%1%)", 0, pol);
}

}}} // namespaces

#endif

