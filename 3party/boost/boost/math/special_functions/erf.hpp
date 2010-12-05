//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_ERF_HPP
#define BOOST_MATH_SPECIAL_ERF_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/policies/error_handling.hpp>

namespace boost{ namespace math{

namespace detail
{

//
// Asymptotic series for large z:
//
template <class T>
struct erf_asympt_series_t
{
   erf_asympt_series_t(T z) : xx(2 * -z * z), tk(1)
   {
      BOOST_MATH_STD_USING
      result = -exp(-z * z) / sqrt(boost::math::constants::pi<T>());
      result /= z;
   }

   typedef T result_type;

   T operator()()
   {
      BOOST_MATH_STD_USING
      T r = result;
      result *= tk / xx;
      tk += 2;
      if( fabs(r) < fabs(result))
         result = 0;
      return r;
   }
private:
   T result;
   T xx;
   int tk;
};
//
// How large z has to be in order to ensure that the series converges:
//
template <class T>
inline float erf_asymptotic_limit_N(const T&)
{
   return (std::numeric_limits<float>::max)();
}
inline float erf_asymptotic_limit_N(const mpl::int_<24>&)
{
   return 2.8F;
}
inline float erf_asymptotic_limit_N(const mpl::int_<53>&)
{
   return 4.3F;
}
inline float erf_asymptotic_limit_N(const mpl::int_<64>&)
{
   return 4.8F;
}
inline float erf_asymptotic_limit_N(const mpl::int_<106>&)
{
   return 6.5F;
}
inline float erf_asymptotic_limit_N(const mpl::int_<113>&)
{
   return 6.8F;
}

template <class T, class Policy>
inline T erf_asymptotic_limit()
{
   typedef typename policies::precision<T, Policy>::type precision_type;
   typedef typename mpl::if_<
      mpl::less_equal<precision_type, mpl::int_<24> >,
      typename mpl::if_<
         mpl::less_equal<precision_type, mpl::int_<0> >,
         mpl::int_<0>,
         mpl::int_<24>
      >::type,
      typename mpl::if_<
         mpl::less_equal<precision_type, mpl::int_<53> >,
         mpl::int_<53>,
         typename mpl::if_<
            mpl::less_equal<precision_type, mpl::int_<64> >,
            mpl::int_<64>,
            typename mpl::if_<
               mpl::less_equal<precision_type, mpl::int_<106> >,
               mpl::int_<106>,
               typename mpl::if_<
                  mpl::less_equal<precision_type, mpl::int_<113> >,
                  mpl::int_<113>,
                  mpl::int_<0>
               >::type
            >::type
         >::type
      >::type
   >::type tag_type;
   return erf_asymptotic_limit_N(tag_type());
}

template <class T, class Policy, class Tag>
T erf_imp(T z, bool invert, const Policy& pol, const Tag& t)
{
   BOOST_MATH_STD_USING

   BOOST_MATH_INSTRUMENT_CODE("Generic erf_imp called");

   if(z < 0)
   {
      if(!invert)
         return -erf_imp(T(-z), invert, pol, t);
      else
         return 1 + erf_imp(T(-z), false, pol, t);
   }

   T result;

   if(!invert && (z > detail::erf_asymptotic_limit<T, Policy>()))
   {
      detail::erf_asympt_series_t<T> s(z);
      boost::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
      result = boost::math::tools::sum_series(s, policies::get_epsilon<T, Policy>(), max_iter, 1);
      policies::check_series_iterations("boost::math::erf<%1%>(%1%, %1%)", max_iter, pol);
   }
   else
   {
      T x = z * z;
      if(x < 0.6)
      {
         // Compute P:
         result = z * exp(-x);
         result /= sqrt(boost::math::constants::pi<T>());
         if(result != 0)
            result *= 2 * detail::lower_gamma_series(T(0.5f), x, pol);
      }
      else if(x < 1.1f)
      {
         // Compute Q:
         invert = !invert;
         result = tgamma_small_upper_part(T(0.5f), x, pol);
         result /= sqrt(boost::math::constants::pi<T>());
      }
      else
      {
         // Compute Q:
         invert = !invert;
         result = z * exp(-x);
         result /= sqrt(boost::math::constants::pi<T>());
         result *= upper_gamma_fraction(T(0.5f), x, policies::get_epsilon<T, Policy>());
      }
   }
   if(invert)
      result = 1 - result;
   return result;
}

template <class T, class Policy>
T erf_imp(T z, bool invert, const Policy& pol, const mpl::int_<53>& t)
{
   BOOST_MATH_STD_USING

   BOOST_MATH_INSTRUMENT_CODE("53-bit precision erf_imp called");

   if(z < 0)
   {
      if(!invert)
         return -erf_imp(-z, invert, pol, t);
      else if(z < -0.5)
         return 2 - erf_imp(-z, invert, pol, t);
      else
         return 1 + erf_imp(-z, false, pol, t);
   }

   T result;

   //
   // Big bunch of selection statements now to pick
   // which implementation to use,
   // try to put most likely options first:
   //
   if(z < 0.5)
   {
      //
      // We're going to calculate erf:
      //
      if(z < 1e-10)
      {
         if(z == 0)
         {
            result = T(0);
         }
         else
         {
            result = static_cast<T>(z * 1.125f + z * 0.003379167095512573896158903121545171688L);
         }
      }
      else
      {
         // Maximum Deviation Found:                     1.561e-17
         // Expected Error Term:                         1.561e-17
         // Maximum Relative Change in Control Points:   1.155e-04
         // Max Error found at double precision =        2.961182e-17

         static const T Y = 1.044948577880859375f;
         static const T P[] = {    
            0.0834305892146531832907L,
            -0.338165134459360935041L,
            -0.0509990735146777432841L,
            -0.00772758345802133288487L,
            -0.000322780120964605683831L,
         };
         static const T Q[] = {    
            1L,
            0.455004033050794024546L,
            0.0875222600142252549554L,
            0.00858571925074406212772L,
            0.000370900071787748000569L,
         };
         T zz = z * z;
         result = z * (Y + tools::evaluate_polynomial(P, zz) / tools::evaluate_polynomial(Q, zz));
      }
   }
   else if(invert ? (z < 28) : (z < 5.8f))
   {
      //
      // We'll be calculating erfc:
      //
      invert = !invert;
      if(z < 1.5f)
      {
         // Maximum Deviation Found:                     3.702e-17
         // Expected Error Term:                         3.702e-17
         // Maximum Relative Change in Control Points:   2.845e-04
         // Max Error found at double precision =        4.841816e-17
         static const T Y = 0.405935764312744140625f;
         static const T P[] = {    
            -0.098090592216281240205L,
            0.178114665841120341155L,
            0.191003695796775433986L,
            0.0888900368967884466578L,
            0.0195049001251218801359L,
            0.00180424538297014223957L,
         };
         static const T Q[] = {    
            1L,
            1.84759070983002217845L,
            1.42628004845511324508L,
            0.578052804889902404909L,
            0.12385097467900864233L,
            0.0113385233577001411017L,
            0.337511472483094676155e-5L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 0.5) / tools::evaluate_polynomial(Q, z - 0.5);
         result *= exp(-z * z) / z;
      }
      else if(z < 2.5f)
      {
         // Max Error found at double precision =        6.599585e-18
         // Maximum Deviation Found:                     3.909e-18
         // Expected Error Term:                         3.909e-18
         // Maximum Relative Change in Control Points:   9.886e-05
         static const T Y = 0.50672817230224609375f;
         static const T P[] = {    
            -0.0243500476207698441272L,
            0.0386540375035707201728L,
            0.04394818964209516296L,
            0.0175679436311802092299L,
            0.00323962406290842133584L,
            0.000235839115596880717416L,
         };
         static const T Q[] = {    
            1L,
            1.53991494948552447182L,
            0.982403709157920235114L,
            0.325732924782444448493L,
            0.0563921837420478160373L,
            0.00410369723978904575884L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 1.5) / tools::evaluate_polynomial(Q, z - 1.5);
         result *= exp(-z * z) / z;
      }
      else if(z < 4.5f)
      {
         // Maximum Deviation Found:                     1.512e-17
         // Expected Error Term:                         1.512e-17
         // Maximum Relative Change in Control Points:   2.222e-04
         // Max Error found at double precision =        2.062515e-17
         static const T Y = 0.5405750274658203125f;
         static const T P[] = {    
            0.00295276716530971662634L,
            0.0137384425896355332126L,
            0.00840807615555585383007L,
            0.00212825620914618649141L,
            0.000250269961544794627958L,
            0.113212406648847561139e-4L,
         };
         static const T Q[] = {    
            1L,
            1.04217814166938418171L,
            0.442597659481563127003L,
            0.0958492726301061423444L,
            0.0105982906484876531489L,
            0.000479411269521714493907L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 3.5) / tools::evaluate_polynomial(Q, z - 3.5);
         result *= exp(-z * z) / z;
      }
      else
      {
         // Max Error found at double precision =        2.997958e-17
         // Maximum Deviation Found:                     2.860e-17
         // Expected Error Term:                         2.859e-17
         // Maximum Relative Change in Control Points:   1.357e-05
         static const T Y = 0.5579090118408203125f;
         static const T P[] = {    
            0.00628057170626964891937L,
            0.0175389834052493308818L,
            -0.212652252872804219852L,
            -0.687717681153649930619L,
            -2.5518551727311523996L,
            -3.22729451764143718517L,
            -2.8175401114513378771L,
         };
         static const T Q[] = {    
            1L,
            2.79257750980575282228L,
            11.0567237927800161565L,
            15.930646027911794143L,
            22.9367376522880577224L,
            13.5064170191802889145L,
            5.48409182238641741584L,
         };
         result = Y + tools::evaluate_polynomial(P, 1 / z) / tools::evaluate_polynomial(Q, 1 / z);
         result *= exp(-z * z) / z;
      }
   }
   else
   {
      //
      // Any value of z larger than 28 will underflow to zero:
      //
      result = 0;
      invert = !invert;
   }

   if(invert)
   {
      result = 1 - result;
   }

   return result;
} // template <class T, class L>T erf_imp(T z, bool invert, const L& l, const mpl::int_<53>& t)


template <class T, class Policy>
T erf_imp(T z, bool invert, const Policy& pol, const mpl::int_<64>& t)
{
   BOOST_MATH_STD_USING

   BOOST_MATH_INSTRUMENT_CODE("64-bit precision erf_imp called");

   if(z < 0)
   {
      if(!invert)
         return -erf_imp(-z, invert, pol, t);
      else if(z < -0.5)
         return 2 - erf_imp(-z, invert, pol, t);
      else
         return 1 + erf_imp(-z, false, pol, t);
   }

   T result;

   //
   // Big bunch of selection statements now to pick which
   // implementation to use, try to put most likely options
   // first:
   //
   if(z < 0.5)
   {
      //
      // We're going to calculate erf:
      //
      if(z == 0)
      {
         result = 0;
      }
      else if(z < 1e-10)
      {
         result = z * 1.125 + z * 0.003379167095512573896158903121545171688L;
      }
      else
      {
         // Max Error found at long double precision =   1.623299e-20
         // Maximum Deviation Found:                     4.326e-22
         // Expected Error Term:                         -4.326e-22
         // Maximum Relative Change in Control Points:   1.474e-04
         static const T Y = 1.044948577880859375f;
         static const T P[] = {    
            0.0834305892146531988966L,
            -0.338097283075565413695L,
            -0.0509602734406067204596L,
            -0.00904906346158537794396L,
            -0.000489468651464798669181L,
            -0.200305626366151877759e-4L,
         };
         static const T Q[] = {    
            1L,
            0.455817300515875172439L,
            0.0916537354356241792007L,
            0.0102722652675910031202L,
            0.000650511752687851548735L,
            0.189532519105655496778e-4L,
         };
         result = z * (Y + tools::evaluate_polynomial(P, z * z) / tools::evaluate_polynomial(Q, z * z));
      }
   }
   else if(invert ? (z < 110) : (z < 6.4f))
   {
      //
      // We'll be calculating erfc:
      //
      invert = !invert;
      if(z < 1.5)
      {
         // Max Error found at long double precision =   3.239590e-20
         // Maximum Deviation Found:                     2.241e-20
         // Expected Error Term:                         -2.241e-20
         // Maximum Relative Change in Control Points:   5.110e-03
         static const T Y = 0.405935764312744140625f;
         static const T P[] = {    
            -0.0980905922162812031672L,
            0.159989089922969141329L,
            0.222359821619935712378L,
            0.127303921703577362312L,
            0.0384057530342762400273L,
            0.00628431160851156719325L,
            0.000441266654514391746428L,
            0.266689068336295642561e-7L,
         };
         static const T Q[] = {    
            1L,
            2.03237474985469469291L,
            1.78355454954969405222L,
            0.867940326293760578231L,
            0.248025606990021698392L,
            0.0396649631833002269861L,
            0.00279220237309449026796L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 0.5f) / tools::evaluate_polynomial(Q, z - 0.5f);
         result *= exp(-z * z) / z;
      }
      else if(z < 2.5)
      {
         // Max Error found at long double precision =   3.686211e-21
         // Maximum Deviation Found:                     1.495e-21
         // Expected Error Term:                         -1.494e-21
         // Maximum Relative Change in Control Points:   1.793e-04
         static const T Y = 0.50672817230224609375f;
         static const T P[] = {    
            -0.024350047620769840217L,
            0.0343522687935671451309L,
            0.0505420824305544949541L,
            0.0257479325917757388209L,
            0.00669349844190354356118L,
            0.00090807914416099524444L,
            0.515917266698050027934e-4L,
         };
         static const T Q[] = {    
            1L,
            1.71657861671930336344L,
            1.26409634824280366218L,
            0.512371437838969015941L,
            0.120902623051120950935L,
            0.0158027197831887485261L,
            0.000897871370778031611439L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 1.5f) / tools::evaluate_polynomial(Q, z - 1.5f);
         result *= exp(-z * z) / z;
      }
      else if(z < 4.5)
      {
         // Maximum Deviation Found:                     1.107e-20
         // Expected Error Term:                         -1.106e-20
         // Maximum Relative Change in Control Points:   1.709e-04
         // Max Error found at long double precision =   1.446908e-20
         static const T Y  = 0.5405750274658203125f;
         static const T P[] = {    
            0.0029527671653097284033L,
            0.0141853245895495604051L,
            0.0104959584626432293901L,
            0.00343963795976100077626L,
            0.00059065441194877637899L,
            0.523435380636174008685e-4L,
            0.189896043050331257262e-5L,
         };
         static const T Q[] = {    
            1L,
            1.19352160185285642574L,
            0.603256964363454392857L,
            0.165411142458540585835L,
            0.0259729870946203166468L,
            0.00221657568292893699158L,
            0.804149464190309799804e-4L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 3.5f) / tools::evaluate_polynomial(Q, z - 3.5f);
         result *= exp(-z * z) / z;
      }
      else
      {
         // Max Error found at long double precision =   7.961166e-21
         // Maximum Deviation Found:                     6.677e-21
         // Expected Error Term:                         6.676e-21
         // Maximum Relative Change in Control Points:   2.319e-05
         static const T Y = 0.55825519561767578125f;
         static const T P[] = {    
            0.00593438793008050214106L,
            0.0280666231009089713937L,
            -0.141597835204583050043L,
            -0.978088201154300548842L,
            -5.47351527796012049443L,
            -13.8677304660245326627L,
            -27.1274948720539821722L,
            -29.2545152747009461519L,
            -16.8865774499799676937L,
         };
         static const T Q[] = {    
            1L,
            4.72948911186645394541L,
            23.6750543147695749212L,
            60.0021517335693186785L,
            131.766251645149522868L,
            178.167924971283482513L,
            182.499390505915222699L,
            104.365251479578577989L,
            30.8365511891224291717L,
         };
         result = Y + tools::evaluate_polynomial(P, 1 / z) / tools::evaluate_polynomial(Q, 1 / z);
         result *= exp(-z * z) / z;
      }
   }
   else
   {
      //
      // Any value of z larger than 110 will underflow to zero:
      //
      result = 0;
      invert = !invert;
   }

   if(invert)
   {
      result = 1 - result;
   }

   return result;
} // template <class T, class L>T erf_imp(T z, bool invert, const L& l, const mpl::int_<64>& t)


template <class T, class Policy>
T erf_imp(T z, bool invert, const Policy& pol, const mpl::int_<113>& t)
{
   BOOST_MATH_STD_USING

   BOOST_MATH_INSTRUMENT_CODE("113-bit precision erf_imp called");

   if(z < 0)
   {
      if(!invert)
         return -erf_imp(-z, invert, pol, t);
      else if(z < -0.5)
         return 2 - erf_imp(-z, invert, pol, t);
      else
         return 1 + erf_imp(-z, false, pol, t);
   }

   T result;

   //
   // Big bunch of selection statements now to pick which
   // implementation to use, try to put most likely options
   // first:
   //
   if(z < 0.5)
   {
      //
      // We're going to calculate erf:
      //
      if(z == 0)
      {
         result = 0;
      }
      else if(z < 1e-20)
      {
         result = z * 1.125 + z * 0.003379167095512573896158903121545171688L;
      }
      else
      {
         // Max Error found at long double precision =   2.342380e-35
         // Maximum Deviation Found:                     6.124e-36
         // Expected Error Term:                         -6.124e-36
         // Maximum Relative Change in Control Points:   3.492e-10
         static const T Y = 1.0841522216796875f;
         static const T P[] = {    
            0.0442269454158250738961589031215451778L,
            -0.35549265736002144875335323556961233L,
            -0.0582179564566667896225454670863270393L,
            -0.0112694696904802304229950538453123925L,
            -0.000805730648981801146251825329609079099L,
            -0.566304966591936566229702842075966273e-4L,
            -0.169655010425186987820201021510002265e-5L,
            -0.344448249920445916714548295433198544e-7L,
         };
         static const T Q[] = {    
            1L,
            0.466542092785657604666906909196052522L,
            0.100005087012526447295176964142107611L,
            0.0128341535890117646540050072234142603L,
            0.00107150448466867929159660677016658186L,
            0.586168368028999183607733369248338474e-4L,
            0.196230608502104324965623171516808796e-5L,
            0.313388521582925207734229967907890146e-7L,
         };
         result = z * (Y + tools::evaluate_polynomial(P, z * z) / tools::evaluate_polynomial(Q, z * z));
      }
   }
   else if(invert ? (z < 110) : (z < 8.65f))
   {
      //
      // We'll be calculating erfc:
      //
      invert = !invert;
      if(z < 1)
      {
         // Max Error found at long double precision =   3.246278e-35
         // Maximum Deviation Found:                     1.388e-35
         // Expected Error Term:                         1.387e-35
         // Maximum Relative Change in Control Points:   6.127e-05
         static const T Y = 0.371877193450927734375f;
         static const T P[] = {    
            -0.0640320213544647969396032886581290455L,
            0.200769874440155895637857443946706731L,
            0.378447199873537170666487408805779826L,
            0.30521399466465939450398642044975127L,
            0.146890026406815277906781824723458196L,
            0.0464837937749539978247589252732769567L,
            0.00987895759019540115099100165904822903L,
            0.00137507575429025512038051025154301132L,
            0.0001144764551085935580772512359680516L,
            0.436544865032836914773944382339900079e-5L,
         };
         static const T Q[] = {    
            1L,
            2.47651182872457465043733800302427977L,
            2.78706486002517996428836400245547955L,
            1.87295924621659627926365005293130693L,
            0.829375825174365625428280908787261065L,
            0.251334771307848291593780143950311514L,
            0.0522110268876176186719436765734722473L,
            0.00718332151250963182233267040106902368L,
            0.000595279058621482041084986219276392459L,
            0.226988669466501655990637599399326874e-4L,
            0.270666232259029102353426738909226413e-10L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 0.5f) / tools::evaluate_polynomial(Q, z - 0.5f);
         result *= exp(-z * z) / z;
      }
      else if(z < 1.5)
      {
         // Max Error found at long double precision =   2.215785e-35
         // Maximum Deviation Found:                     1.539e-35
         // Expected Error Term:                         1.538e-35
         // Maximum Relative Change in Control Points:   6.104e-05
         static const T Y = 0.45658016204833984375f;
         static const T P[] = {    
            -0.0289965858925328393392496555094848345L,
            0.0868181194868601184627743162571779226L,
            0.169373435121178901746317404936356745L,
            0.13350446515949251201104889028133486L,
            0.0617447837290183627136837688446313313L,
            0.0185618495228251406703152962489700468L,
            0.00371949406491883508764162050169531013L,
            0.000485121708792921297742105775823900772L,
            0.376494706741453489892108068231400061e-4L,
            0.133166058052466262415271732172490045e-5L,
         };
         static const T Q[] = {    
            1L,
            2.32970330146503867261275580968135126L,
            2.46325715420422771961250513514928746L,
            1.55307882560757679068505047390857842L,
            0.644274289865972449441174485441409076L,
            0.182609091063258208068606847453955649L,
            0.0354171651271241474946129665801606795L,
            0.00454060370165285246451879969534083997L,
            0.000349871943711566546821198612518656486L,
            0.123749319840299552925421880481085392e-4L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 1.0f) / tools::evaluate_polynomial(Q, z - 1.0f);
         result *= exp(-z * z) / z;
      }
      else if(z < 2.25)
      {
         // Maximum Deviation Found:                     1.418e-35
         // Expected Error Term:                         1.418e-35
         // Maximum Relative Change in Control Points:   1.316e-04
         // Max Error found at long double precision =   1.998462e-35
         static const T Y = 0.50250148773193359375f;
         static const T P[] = {    
            -0.0201233630504573402185161184151016606L,
            0.0331864357574860196516686996302305002L,
            0.0716562720864787193337475444413405461L,
            0.0545835322082103985114927569724880658L,
            0.0236692635189696678976549720784989593L,
            0.00656970902163248872837262539337601845L,
            0.00120282643299089441390490459256235021L,
            0.000142123229065182650020762792081622986L,
            0.991531438367015135346716277792989347e-5L,
            0.312857043762117596999398067153076051e-6L,
         };
         static const T Q[] = {    
            1L,
            2.13506082409097783827103424943508554L,
            2.06399257267556230937723190496806215L,
            1.18678481279932541314830499880691109L,
            0.447733186643051752513538142316799562L,
            0.11505680005657879437196953047542148L,
            0.020163993632192726170219663831914034L,
            0.00232708971840141388847728782209730585L,
            0.000160733201627963528519726484608224112L,
            0.507158721790721802724402992033269266e-5L,
            0.18647774409821470950544212696270639e-12L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 1.5f) / tools::evaluate_polynomial(Q, z - 1.5f);
         result *= exp(-z * z) / z;
      }
      else if (z < 3)
      {
         // Maximum Deviation Found:                     3.575e-36
         // Expected Error Term:                         3.575e-36
         // Maximum Relative Change in Control Points:   7.103e-05
         // Max Error found at long double precision =   5.794737e-36
         static const T Y = 0.52896785736083984375f;
         static const T P[] = {    
            -0.00902152521745813634562524098263360074L,
            0.0145207142776691539346923710537580927L,
            0.0301681239582193983824211995978678571L,
            0.0215548540823305814379020678660434461L,
            0.00864683476267958365678294164340749949L,
            0.00219693096885585491739823283511049902L,
            0.000364961639163319762492184502159894371L,
            0.388174251026723752769264051548703059e-4L,
            0.241918026931789436000532513553594321e-5L,
            0.676586625472423508158937481943649258e-7L,
         };
         static const T Q[] = {    
            1L,
            1.93669171363907292305550231764920001L,
            1.69468476144051356810672506101377494L,
            0.880023580986436640372794392579985511L,
            0.299099106711315090710836273697708402L,
            0.0690593962363545715997445583603382337L,
            0.0108427016361318921960863149875360222L,
            0.00111747247208044534520499324234317695L,
            0.686843205749767250666787987163701209e-4L,
            0.192093541425429248675532015101904262e-5L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 2.25f) / tools::evaluate_polynomial(Q, z - 2.25f);
         result *= exp(-z * z) / z;
      }
      else if(z < 3.5)
      {
         // Maximum Deviation Found:                     8.126e-37
         // Expected Error Term:                         -8.126e-37
         // Maximum Relative Change in Control Points:   1.363e-04
         // Max Error found at long double precision =   1.747062e-36
         static const T Y = 0.54037380218505859375f;
         static const T P[] = {    
            -0.0033703486408887424921155540591370375L,
            0.0104948043110005245215286678898115811L,
            0.0148530118504000311502310457390417795L,
            0.00816693029245443090102738825536188916L,
            0.00249716579989140882491939681805594585L,
            0.0004655591010047353023978045800916647L,
            0.531129557920045295895085236636025323e-4L,
            0.343526765122727069515775194111741049e-5L,
            0.971120407556888763695313774578711839e-7L,
         };
         static const T Q[] = {    
            1L,
            1.59911256167540354915906501335919317L,
            1.136006830764025173864831382946934L,
            0.468565867990030871678574840738423023L,
            0.122821824954470343413956476900662236L,
            0.0209670914950115943338996513330141633L,
            0.00227845718243186165620199012883547257L,
            0.000144243326443913171313947613547085553L,
            0.407763415954267700941230249989140046e-5L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 3.0f) / tools::evaluate_polynomial(Q, z - 3.0f);
         result *= exp(-z * z) / z;
      }
      else if(z < 5.5)
      {
         // Maximum Deviation Found:                     5.804e-36
         // Expected Error Term:                         -5.803e-36
         // Maximum Relative Change in Control Points:   2.475e-05
         // Max Error found at long double precision =   1.349545e-35
         static const T Y = 0.55000019073486328125f;
         static const T P[] = {    
            0.00118142849742309772151454518093813615L,
            0.0072201822885703318172366893469382745L,
            0.0078782276276860110721875733778481505L,
            0.00418229166204362376187593976656261146L,
            0.00134198400587769200074194304298642705L,
            0.000283210387078004063264777611497435572L,
            0.405687064094911866569295610914844928e-4L,
            0.39348283801568113807887364414008292e-5L,
            0.248798540917787001526976889284624449e-6L,
            0.929502490223452372919607105387474751e-8L,
            0.156161469668275442569286723236274457e-9L,
         };
         static const T Q[] = {    
            1L,
            1.52955245103668419479878456656709381L,
            1.06263944820093830054635017117417064L,
            0.441684612681607364321013134378316463L,
            0.121665258426166960049773715928906382L,
            0.0232134512374747691424978642874321434L,
            0.00310778180686296328582860464875562636L,
            0.000288361770756174705123674838640161693L,
            0.177529187194133944622193191942300132e-4L,
            0.655068544833064069223029299070876623e-6L,
            0.11005507545746069573608988651927452e-7L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 4.5f) / tools::evaluate_polynomial(Q, z - 4.5f);
         result *= exp(-z * z) / z;
      }
      else if(z < 7.5)
      {
         // Maximum Deviation Found:                     1.007e-36
         // Expected Error Term:                         1.007e-36
         // Maximum Relative Change in Control Points:   1.027e-03
         // Max Error found at long double precision =   2.646420e-36
         static const T Y = 0.5574436187744140625f;
         static const T P[] = {    
            0.000293236907400849056269309713064107674L,
            0.00225110719535060642692275221961480162L,
            0.00190984458121502831421717207849429799L,
            0.000747757733460111743833929141001680706L,
            0.000170663175280949889583158597373928096L,
            0.246441188958013822253071608197514058e-4L,
            0.229818000860544644974205957895688106e-5L,
            0.134886977703388748488480980637704864e-6L,
            0.454764611880548962757125070106650958e-8L,
            0.673002744115866600294723141176820155e-10L,
         };
         static const T Q[] = {    
            1L,
            1.12843690320861239631195353379313367L,
            0.569900657061622955362493442186537259L,
            0.169094404206844928112348730277514273L,
            0.0324887449084220415058158657252147063L,
            0.00419252877436825753042680842608219552L,
            0.00036344133176118603523976748563178578L,
            0.204123895931375107397698245752850347e-4L,
            0.674128352521481412232785122943508729e-6L,
            0.997637501418963696542159244436245077e-8L,
         };
         result = Y + tools::evaluate_polynomial(P, z - 6.5f) / tools::evaluate_polynomial(Q, z - 6.5f);
         result *= exp(-z * z) / z;
      }
      else if(z < 11.5)
      {
         // Maximum Deviation Found:                     8.380e-36
         // Expected Error Term:                         8.380e-36
         // Maximum Relative Change in Control Points:   2.632e-06
         // Max Error found at long double precision =   9.849522e-36
         static const T Y = 0.56083202362060546875f;
         static const T P[] = {    
            0.000282420728751494363613829834891390121L,
            0.00175387065018002823433704079355125161L,
            0.0021344978564889819420775336322920375L,
            0.00124151356560137532655039683963075661L,
            0.000423600733566948018555157026862139644L,
            0.914030340865175237133613697319509698e-4L,
            0.126999927156823363353809747017945494e-4L,
            0.110610959842869849776179749369376402e-5L,
            0.55075079477173482096725348704634529e-7L,
            0.119735694018906705225870691331543806e-8L,
         };
         static const T Q[] = {    
            1L,
            1.69889613396167354566098060039549882L,
            1.28824647372749624464956031163282674L,
            0.572297795434934493541628008224078717L,
            0.164157697425571712377043857240773164L,
            0.0315311145224594430281219516531649562L,
            0.00405588922155632380812945849777127458L,
            0.000336929033691445666232029762868642417L,
            0.164033049810404773469413526427932109e-4L,
            0.356615210500531410114914617294694857e-6L,
         };
         result = Y + tools::evaluate_polynomial(P, z / 2 - 4.75f) / tools::evaluate_polynomial(Q, z / 2 - 4.75f);
         result *= exp(-z * z) / z;
      }
      else
      {
         // Maximum Deviation Found:                     1.132e-35
         // Expected Error Term:                         -1.132e-35
         // Maximum Relative Change in Control Points:   4.674e-04
         // Max Error found at long double precision =   1.162590e-35
         static const T Y = 0.5632686614990234375f;
         static const T P[] = {    
            0.000920922048732849448079451574171836943L,
            0.00321439044532288750501700028748922439L,
            -0.250455263029390118657884864261823431L,
            -0.906807635364090342031792404764598142L,
            -8.92233572835991735876688745989985565L,
            -21.7797433494422564811782116907878495L,
            -91.1451915251976354349734589601171659L,
            -144.1279109655993927069052125017673L,
            -313.845076581796338665519022313775589L,
            -273.11378811923343424081101235736475L,
            -271.651566205951067025696102600443452L,
            -60.0530577077238079968843307523245547L,
         };
         static const T Q[] = {    
            1L,
            3.49040448075464744191022350947892036L,
            34.3563592467165971295915749548313227L,
            84.4993232033879023178285731843850461L,
            376.005865281206894120659401340373818L,
            629.95369438888946233003926191755125L,
            1568.35771983533158591604513304269098L,
            1646.02452040831961063640827116581021L,
            2299.96860633240298708910425594484895L,
            1222.73204392037452750381340219906374L,
            799.359797306084372350264298361110448L,
            72.7415265778588087243442792401576737L,
         };
         result = Y + tools::evaluate_polynomial(P, 1 / z) / tools::evaluate_polynomial(Q, 1 / z);
         result *= exp(-z * z) / z;
      }
   }
   else
   {
      //
      // Any value of z larger than 110 will underflow to zero:
      //
      result = 0;
      invert = !invert;
   }

   if(invert)
   {
      result = 1 - result;
   }

   return result;
} // template <class T, class L>T erf_imp(T z, bool invert, const L& l, const mpl::int_<113>& t)

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type erf(T z, const Policy& /* pol */)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   BOOST_MATH_INSTRUMENT_CODE("result_type = " << typeid(result_type).name());
   BOOST_MATH_INSTRUMENT_CODE("value_type = " << typeid(value_type).name());
   BOOST_MATH_INSTRUMENT_CODE("precision_type = " << typeid(precision_type).name());

   typedef typename mpl::if_<
      mpl::less_equal<precision_type, mpl::int_<0> >,
      mpl::int_<0>,
      typename mpl::if_<
         mpl::less_equal<precision_type, mpl::int_<53> >,
         mpl::int_<53>,  // double
         typename mpl::if_<
            mpl::less_equal<precision_type, mpl::int_<64> >,
            mpl::int_<64>, // 80-bit long double
            typename mpl::if_<
               mpl::less_equal<precision_type, mpl::int_<113> >,
               mpl::int_<113>, // 128-bit long double
               mpl::int_<0> // too many bits, use generic version.
            >::type
         >::type
      >::type
   >::type tag_type;

   BOOST_MATH_INSTRUMENT_CODE("tag_type = " << typeid(tag_type).name());

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::erf_imp(
      static_cast<value_type>(z),
      false,
      forwarding_policy(),
      tag_type()), "boost::math::erf<%1%>(%1%, %1%)");
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type erfc(T z, const Policy& /* pol */)
{
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::precision<result_type, Policy>::type precision_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   BOOST_MATH_INSTRUMENT_CODE("result_type = " << typeid(result_type).name());
   BOOST_MATH_INSTRUMENT_CODE("value_type = " << typeid(value_type).name());
   BOOST_MATH_INSTRUMENT_CODE("precision_type = " << typeid(precision_type).name());

   typedef typename mpl::if_<
      mpl::less_equal<precision_type, mpl::int_<0> >,
      mpl::int_<0>,
      typename mpl::if_<
         mpl::less_equal<precision_type, mpl::int_<53> >,
         mpl::int_<53>,  // double
         typename mpl::if_<
            mpl::less_equal<precision_type, mpl::int_<64> >,
            mpl::int_<64>, // 80-bit long double
            typename mpl::if_<
               mpl::less_equal<precision_type, mpl::int_<113> >,
               mpl::int_<113>, // 128-bit long double
               mpl::int_<0> // too many bits, use generic version.
            >::type
         >::type
      >::type
   >::type tag_type;

   BOOST_MATH_INSTRUMENT_CODE("tag_type = " << typeid(tag_type).name());

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::erf_imp(
      static_cast<value_type>(z),
      true,
      forwarding_policy(),
      tag_type()), "boost::math::erfc<%1%>(%1%, %1%)");
}

template <class T>
inline typename tools::promote_args<T>::type erf(T z)
{
   return boost::math::erf(z, policies::policy<>());
}

template <class T>
inline typename tools::promote_args<T>::type erfc(T z)
{
   return boost::math::erfc(z, policies::policy<>());
}

} // namespace math
} // namespace boost

#include <boost/math/special_functions/detail/erf_inv.hpp>

#endif // BOOST_MATH_SPECIAL_ERF_HPP




