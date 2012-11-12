// Copyright John Maddock 2012.
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_AIRY_HPP
#define BOOST_MATH_AIRY_HPP

#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/cbrt.hpp>

namespace boost{ namespace math{

namespace detail{

template <class T, class Policy>
T airy_ai_imp(T x, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(x < 0)
   {
      T p = (-x * sqrt(-x) * 2) / 3;
      T v = T(1) / 3;
      T j1 = boost::math::cyl_bessel_j(v, p, pol);
      T j2 = boost::math::cyl_bessel_j(-v, p, pol);
      T ai = sqrt(-x) * (j1 + j2) / 3;
      //T bi = sqrt(-x / 3) * (j2 - j1);
      return ai;
   }
   else if(fabs(x * x * x) / 6 < tools::epsilon<T>())
   {
      T tg = boost::math::tgamma(constants::twothirds<T>(), pol);
      T ai = 1 / (pow(T(3), constants::twothirds<T>()) * tg);
      //T bi = 1 / (sqrt(boost::math::cbrt(T(3))) * tg);
      return ai;
   }
   else
   {
      T p = 2 * x * sqrt(x) / 3;
      T v = T(1) / 3;
      //T j1 = boost::math::cyl_bessel_i(-v, p, pol);
      //T j2 = boost::math::cyl_bessel_i(v, p, pol);
      //
      // Note that although we can calculate ai from j1 and j2, the accuracy is horrible
      // as we're subtracting two very large values, so use the Bessel K relation instead:
      //
      T ai = cyl_bessel_k(v, p, pol) * sqrt(x / 3) / boost::math::constants::pi<T>();  //sqrt(x) * (j1 - j2) / 3;
      //T bi = sqrt(x / 3) * (j1 + j2);
      return ai;
   }
}

template <class T, class Policy>
T airy_bi_imp(T x, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(x < 0)
   {
      T p = (-x * sqrt(-x) * 2) / 3;
      T v = T(1) / 3;
      T j1 = boost::math::cyl_bessel_j(v, p, pol);
      T j2 = boost::math::cyl_bessel_j(-v, p, pol);
      //T ai = sqrt(-x) * (j1 + j2) / 3;
      T bi = sqrt(-x / 3) * (j2 - j1);
      return bi;
   }
   else if(fabs(x * x * x) / 6 < tools::epsilon<T>())
   {
      T tg = boost::math::tgamma(constants::twothirds<T>(), pol);
      //T ai = 1 / (pow(T(3), constants::twothirds<T>()) * tg);
      T bi = 1 / (sqrt(boost::math::cbrt(T(3))) * tg);
      return bi;
   }
   else
   {
      T p = 2 * x * sqrt(x) / 3;
      T v = T(1) / 3;
      T j1 = boost::math::cyl_bessel_i(-v, p, pol);
      T j2 = boost::math::cyl_bessel_i(v, p, pol);
      T bi = sqrt(x / 3) * (j1 + j2);
      return bi;
   }
}

template <class T, class Policy>
T airy_ai_prime_imp(T x, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(x < 0)
   {
      T p = (-x * sqrt(-x) * 2) / 3;
      T v = T(2) / 3;
      T j1 = boost::math::cyl_bessel_j(v, p, pol);
      T j2 = boost::math::cyl_bessel_j(-v, p, pol);
      T aip = -x * (j1 - j2) / 3;
      return aip;
   }
   else if(fabs(x * x) / 2 < tools::epsilon<T>())
   {
      T tg = boost::math::tgamma(constants::third<T>(), pol);
      T aip = 1 / (boost::math::cbrt(T(3)) * tg);
      return -aip;
   }
   else
   {
      T p = 2 * x * sqrt(x) / 3;
      T v = T(2) / 3;
      //T j1 = boost::math::cyl_bessel_i(-v, p, pol);
      //T j2 = boost::math::cyl_bessel_i(v, p, pol);
      //
      // Note that although we can calculate ai from j1 and j2, the accuracy is horrible
      // as we're subtracting two very large values, so use the Bessel K relation instead:
      //
      T aip = -cyl_bessel_k(v, p, pol) * x / (boost::math::constants::root_three<T>() * boost::math::constants::pi<T>());
      return aip;
   }
}

template <class T, class Policy>
T airy_bi_prime_imp(T x, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(x < 0)
   {
      T p = (-x * sqrt(-x) * 2) / 3;
      T v = T(2) / 3;
      T j1 = boost::math::cyl_bessel_j(v, p, pol);
      T j2 = boost::math::cyl_bessel_j(-v, p, pol);
      T aip = -x * (j1 + j2) / constants::root_three<T>();
      return aip;
   }
   else if(fabs(x * x) / 2 < tools::epsilon<T>())
   {
      T tg = boost::math::tgamma(constants::third<T>(), pol);
      T bip = sqrt(boost::math::cbrt(T(3))) / tg;
      return bip;
   }
   else
   {
      T p = 2 * x * sqrt(x) / 3;
      T v = T(2) / 3;
      T j1 = boost::math::cyl_bessel_i(-v, p, pol);
      T j2 = boost::math::cyl_bessel_i(v, p, pol);
      T aip = x * (j1 + j2) / boost::math::constants::root_three<T>();
      return aip;
   }
}

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type airy_ai(T x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, Policy>(detail::airy_ai_imp<value_type>(static_cast<value_type>(x), forwarding_policy()), "boost::math::airy<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type airy_ai(T x)
{
   return airy_ai(x, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type airy_bi(T x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, Policy>(detail::airy_bi_imp<value_type>(static_cast<value_type>(x), forwarding_policy()), "boost::math::airy<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type airy_bi(T x)
{
   return airy_bi(x, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type airy_ai_prime(T x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, Policy>(detail::airy_ai_prime_imp<value_type>(static_cast<value_type>(x), forwarding_policy()), "boost::math::airy<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type airy_ai_prime(T x)
{
   return airy_ai_prime(x, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type airy_bi_prime(T x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, Policy>(detail::airy_bi_prime_imp<value_type>(static_cast<value_type>(x), forwarding_policy()), "boost::math::airy<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type airy_bi_prime(T x)
{
   return airy_bi_prime(x, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_AIRY_HPP
