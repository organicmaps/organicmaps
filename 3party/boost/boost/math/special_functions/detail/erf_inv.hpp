//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SF_ERF_INV_HPP
#define BOOST_MATH_SF_ERF_INV_HPP

#ifdef _MSC_VER
#pragma once
#endif

namespace boost{ namespace math{ 

namespace detail{
//
// The inverse erf and erfc functions share a common implementation,
// this version is for 80-bit long double's and smaller:
//
template <class T, class Policy>
T erf_inv_imp(const T& p, const T& q, const Policy&, const boost::mpl::int_<64>*)
{
   BOOST_MATH_STD_USING // for ADL of std names.

   T result = 0;
   
   if(p <= 0.5)
   {
      //
      // Evaluate inverse erf using the rational approximation:
      //
      // x = p(p+10)(Y+R(p))
      //
      // Where Y is a constant, and R(p) is optimised for a low
      // absolute error compared to |Y|.
      //
      // double: Max error found: 2.001849e-18
      // long double: Max error found: 1.017064e-20
      // Maximum Deviation Found (actual error term at infinite precision) 8.030e-21
      //
      static const float Y = 0.0891314744949340820313f;
      static const T P[] = {    
         -0.000508781949658280665617L,
         -0.00836874819741736770379L,
         0.0334806625409744615033L,
         -0.0126926147662974029034L,
         -0.0365637971411762664006L,
         0.0219878681111168899165L,
         0.00822687874676915743155L,
         -0.00538772965071242932965L
      };
      static const T Q[] = {    
         1,
         -0.970005043303290640362L,
         -1.56574558234175846809L,
         1.56221558398423026363L,
         0.662328840472002992063L,
         -0.71228902341542847553L,
         -0.0527396382340099713954L,
         0.0795283687341571680018L,
         -0.00233393759374190016776L,
         0.000886216390456424707504L
      };
      T g = p * (p + 10);
      T r = tools::evaluate_polynomial(P, p) / tools::evaluate_polynomial(Q, p);
      result = g * Y + g * r;
   }
   else if(q >= 0.25)
   {
      //
      // Rational approximation for 0.5 > q >= 0.25
      //
      // x = sqrt(-2*log(q)) / (Y + R(q))
      //
      // Where Y is a constant, and R(q) is optimised for a low
      // absolute error compared to Y.
      //
      // double : Max error found: 7.403372e-17
      // long double : Max error found: 6.084616e-20
      // Maximum Deviation Found (error term) 4.811e-20
      //
      static const float Y = 2.249481201171875f;
      static const T P[] = {    
         -0.202433508355938759655L,
         0.105264680699391713268L,
         8.37050328343119927838L,
         17.6447298408374015486L,
         -18.8510648058714251895L,
         -44.6382324441786960818L,
         17.445385985570866523L,
         21.1294655448340526258L,
         -3.67192254707729348546L
      };
      static const T Q[] = {    
         1L,
         6.24264124854247537712L,
         3.9713437953343869095L,
         -28.6608180499800029974L,
         -20.1432634680485188801L,
         48.5609213108739935468L,
         10.8268667355460159008L,
         -22.6436933413139721736L,
         1.72114765761200282724L
      };
      T g = sqrt(-2 * log(q));
      T xs = q - 0.25;
      T r = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
      result = g / (Y + r);
   }
   else
   {
      //
      // For q < 0.25 we have a series of rational approximations all
      // of the general form:
      //
      // let: x = sqrt(-log(q))
      //
      // Then the result is given by:
      //
      // x(Y+R(x-B))
      //
      // where Y is a constant, B is the lowest value of x for which 
      // the approximation is valid, and R(x-B) is optimised for a low
      // absolute error compared to Y.
      //
      // Note that almost all code will really go through the first
      // or maybe second approximation.  After than we're dealing with very
      // small input values indeed: 80 and 128 bit long double's go all the
      // way down to ~ 1e-5000 so the "tail" is rather long...
      //
      T x = sqrt(-log(q));
      if(x < 3)
      {
         // Max error found: 1.089051e-20
         static const float Y = 0.807220458984375f;
         static const T P[] = {    
            -0.131102781679951906451L,
            -0.163794047193317060787L,
            0.117030156341995252019L,
            0.387079738972604337464L,
            0.337785538912035898924L,
            0.142869534408157156766L,
            0.0290157910005329060432L,
            0.00214558995388805277169L,
            -0.679465575181126350155e-6L,
            0.285225331782217055858e-7L,
            -0.681149956853776992068e-9L
         };
         static const T Q[] = {    
            1,
            3.46625407242567245975L,
            5.38168345707006855425L,
            4.77846592945843778382L,
            2.59301921623620271374L,
            0.848854343457902036425L,
            0.152264338295331783612L,
            0.01105924229346489121L
         };
         T xs = x - 1.125;
         T R = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
         result = Y * x + R * x;
      }
      else if(x < 6)
      {
         // Max error found: 8.389174e-21
         static const float Y = 0.93995571136474609375f;
         static const T P[] = {    
            -0.0350353787183177984712L,
            -0.00222426529213447927281L,
            0.0185573306514231072324L,
            0.00950804701325919603619L,
            0.00187123492819559223345L,
            0.000157544617424960554631L,
            0.460469890584317994083e-5L,
            -0.230404776911882601748e-9L,
            0.266339227425782031962e-11L
         };
         static const T Q[] = {    
            1L,
            1.3653349817554063097L,
            0.762059164553623404043L,
            0.220091105764131249824L,
            0.0341589143670947727934L,
            0.00263861676657015992959L,
            0.764675292302794483503e-4L
         };
         T xs = x - 3;
         T R = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
         result = Y * x + R * x;
      }
      else if(x < 18)
      {
         // Max error found: 1.481312e-19
         static const float Y = 0.98362827301025390625f;
         static const T P[] = {    
            -0.0167431005076633737133L,
            -0.00112951438745580278863L,
            0.00105628862152492910091L,
            0.000209386317487588078668L,
            0.149624783758342370182e-4L,
            0.449696789927706453732e-6L,
            0.462596163522878599135e-8L,
            -0.281128735628831791805e-13L,
            0.99055709973310326855e-16L
         };
         static const T Q[] = {    
            1L,
            0.591429344886417493481L,
            0.138151865749083321638L,
            0.0160746087093676504695L,
            0.000964011807005165528527L,
            0.275335474764726041141e-4L,
            0.282243172016108031869e-6L
         };
         T xs = x - 6;
         T R = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
         result = Y * x + R * x;
      }
      else if(x < 44)
      {
         // Max error found: 5.697761e-20
         static const float Y = 0.99714565277099609375f;
         static const T P[] = {    
            -0.0024978212791898131227L,
            -0.779190719229053954292e-5L,
            0.254723037413027451751e-4L,
            0.162397777342510920873e-5L,
            0.396341011304801168516e-7L,
            0.411632831190944208473e-9L,
            0.145596286718675035587e-11L,
            -0.116765012397184275695e-17L
         };
         static const T Q[] = {    
            1L,
            0.207123112214422517181L,
            0.0169410838120975906478L,
            0.000690538265622684595676L,
            0.145007359818232637924e-4L,
            0.144437756628144157666e-6L,
            0.509761276599778486139e-9L
         };
         T xs = x - 18;
         T R = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
         result = Y * x + R * x;
      }
      else
      {
         // Max error found: 1.279746e-20
         static const float Y = 0.99941349029541015625f;
         static const T P[] = {    
            -0.000539042911019078575891L,
            -0.28398759004727721098e-6L,
            0.899465114892291446442e-6L,
            0.229345859265920864296e-7L,
            0.225561444863500149219e-9L,
            0.947846627503022684216e-12L,
            0.135880130108924861008e-14L,
            -0.348890393399948882918e-21L
         };
         static const T Q[] = {    
            1L,
            0.0845746234001899436914L,
            0.00282092984726264681981L,
            0.468292921940894236786e-4L,
            0.399968812193862100054e-6L,
            0.161809290887904476097e-8L,
            0.231558608310259605225e-11L
         };
         T xs = x - 44;
         T R = tools::evaluate_polynomial(P, xs) / tools::evaluate_polynomial(Q, xs);
         result = Y * x + R * x;
      }
   }
   return result;
}

template <class T, class Policy>
struct erf_roots
{
   boost::math::tuple<T,T,T> operator()(const T& guess)
   {
      BOOST_MATH_STD_USING
      T derivative = sign * (2 / sqrt(constants::pi<T>())) * exp(-(guess * guess));
      T derivative2 = -2 * guess * derivative;
      return boost::math::make_tuple(((sign > 0) ? boost::math::erf(guess, Policy()) : boost::math::erfc(guess, Policy())) - target, derivative, derivative2);
   }
   erf_roots(T z, int s) : target(z), sign(s) {}
private:
   T target;
   int sign;
};

template <class T, class Policy>
T erf_inv_imp(const T& p, const T& q, const Policy& pol, const boost::mpl::int_<0>*)
{
   //
   // Generic version, get a guess that's accurate to 64-bits (10^-19)
   //
   T guess = erf_inv_imp(p, q, pol, static_cast<mpl::int_<64> const*>(0));
   T result;
   //
   // If T has more bit's than 64 in it's mantissa then we need to iterate,
   // otherwise we can just return the result:
   //
   if(policies::digits<T, Policy>() > 64)
   {
      boost::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
      if(p <= 0.5)
      {
         result = tools::halley_iterate(detail::erf_roots<typename remove_cv<T>::type, Policy>(p, 1), guess, static_cast<T>(0), tools::max_value<T>(), (policies::digits<T, Policy>() * 2) / 3, max_iter);
      }
      else
      {
         result = tools::halley_iterate(detail::erf_roots<typename remove_cv<T>::type, Policy>(q, -1), guess, static_cast<T>(0), tools::max_value<T>(), (policies::digits<T, Policy>() * 2) / 3, max_iter);
      }
      policies::check_root_iterations("boost::math::erf_inv<%1%>", max_iter, pol);
   }
   else
   {
      result = guess;
   }
   return result;
}

} // namespace detail

template <class T, class Policy>
typename tools::promote_args<T>::type erfc_inv(T z, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   //
   // Begin by testing for domain errors, and other special cases:
   //
   static const char* function = "boost::math::erfc_inv<%1%>(%1%, %1%)";
   if((z < 0) || (z > 2))
      policies::raise_domain_error<result_type>(function, "Argument outside range [0,2] in inverse erfc function (got p=%1%).", z, pol);
   if(z == 0)
      return policies::raise_overflow_error<result_type>(function, 0, pol);
   if(z == 2)
      return -policies::raise_overflow_error<result_type>(function, 0, pol);
   //
   // Normalise the input, so it's in the range [0,1], we will
   // negate the result if z is outside that range.  This is a simple
   // application of the erfc reflection formula: erfc(-z) = 2 - erfc(z)
   //
   result_type p, q, s;
   if(z > 1)
   {
      q = 2 - z;
      p = 1 - q;
      s = -1;
   }
   else
   {
      p = 1 - z;
      q = z;
      s = 1;
   }
   //
   // A bit of meta-programming to figure out which implementation
   // to use, based on the number of bits in the mantissa of T:
   //
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename mpl::if_<
      mpl::or_<mpl::less_equal<precision_type, mpl::int_<0> >, mpl::greater<precision_type, mpl::int_<64> > >,
      mpl::int_<0>,
      mpl::int_<64>
   >::type tag_type;
   //
   // Likewise use internal promotion, so we evaluate at a higher
   // precision internally if it's appropriate:
   //
   typedef typename policies::evaluation<result_type, Policy>::type eval_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   //
   // And get the result, negating where required:
   //
   return s * policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::erf_inv_imp(static_cast<eval_type>(p), static_cast<eval_type>(q), forwarding_policy(), static_cast<tag_type const*>(0)), function);
}

template <class T, class Policy>
typename tools::promote_args<T>::type erf_inv(T z, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   //
   // Begin by testing for domain errors, and other special cases:
   //
   static const char* function = "boost::math::erf_inv<%1%>(%1%, %1%)";
   if((z < -1) || (z > 1))
      policies::raise_domain_error<result_type>(function, "Argument outside range [-1, 1] in inverse erf function (got p=%1%).", z, pol);
   if(z == 1)
      return policies::raise_overflow_error<result_type>(function, 0, pol);
   if(z == -1)
      return -policies::raise_overflow_error<result_type>(function, 0, pol);
   if(z == 0)
      return 0;
   //
   // Normalise the input, so it's in the range [0,1], we will
   // negate the result if z is outside that range.  This is a simple
   // application of the erf reflection formula: erf(-z) = -erf(z)
   //
   result_type p, q, s;
   if(z < 0)
   {
      p = -z;
      q = 1 - p;
      s = -1;
   }
   else
   {
      p = z;
      q = 1 - z;
      s = 1;
   }
   //
   // A bit of meta-programming to figure out which implementation
   // to use, based on the number of bits in the mantissa of T:
   //
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename mpl::if_<
      mpl::or_<mpl::less_equal<precision_type, mpl::int_<0> >, mpl::greater<precision_type, mpl::int_<64> > >,
      mpl::int_<0>,
      mpl::int_<64>
   >::type tag_type;
   //
   // Likewise use internal promotion, so we evaluate at a higher
   // precision internally if it's appropriate:
   //
   typedef typename policies::evaluation<result_type, Policy>::type eval_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   //
   // Likewise use internal promotion, so we evaluate at a higher
   // precision internally if it's appropriate:
   //
   typedef typename policies::evaluation<result_type, Policy>::type eval_type;
   //
   // And get the result, negating where required:
   //
   return s * policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::erf_inv_imp(static_cast<eval_type>(p), static_cast<eval_type>(q), forwarding_policy(), static_cast<tag_type const*>(0)), function);
}

template <class T>
inline typename tools::promote_args<T>::type erfc_inv(T z)
{
   return erfc_inv(z, policies::policy<>());
}

template <class T>
inline typename tools::promote_args<T>::type erf_inv(T z)
{
   return erf_inv(z, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SF_ERF_INV_HPP

