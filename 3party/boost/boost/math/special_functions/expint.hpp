//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_EXPINT_HPP
#define BOOST_MATH_EXPINT_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/tools/fraction.hpp>
#include <boost/math/tools/series.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/digamma.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/pow.hpp>

namespace boost{ namespace math{

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint(unsigned n, T z, const Policy& /*pol*/);
   
namespace detail{

template <class T>
inline T expint_1_rational(const T& z, const mpl::int_<0>&)
{
   // this function is never actually called
   BOOST_ASSERT(0);
   return z;
}

template <class T>
T expint_1_rational(const T& z, const mpl::int_<53>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     2.006e-18
      // Expected Error Term:                         2.006e-18
      // Max error found at double precision:         2.760e-17
      static const T Y = 0.66373538970947265625F;
      static const T P[6] = {    
         0.0865197248079397976498L,
         0.0320913665303559189999L,
         -0.245088216639761496153L,
         -0.0368031736257943745142L,
         -0.00399167106081113256961L,
         -0.000111507792921197858394L
      };
      static const T Q[6] = {    
         1L,
         0.37091387659397013215L,
         0.056770677104207528384L,
         0.00427347600017103698101L,
         0.000131049900798434683324L,
         -0.528611029520217142048e-6L
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Maximum Deviation Found (interpolated):      1.444e-17
      // Max error found at double precision:         3.119e-17
      static const T P[11] = {    
         -0.121013190657725568138e-18L,
         -0.999999999999998811143L,
         -43.3058660811817946037L,
         -724.581482791462469795L,
         -6046.8250112711035463L,
         -27182.6254466733970467L,
         -66598.2652345418633509L,
         -86273.1567711649528784L,
         -54844.4587226402067411L,
         -14751.4895786128450662L,
         -1185.45720315201027667L
      };
      static const T Q[12] = {    
         1L,
         45.3058660811801465927L,
         809.193214954550328455L,
         7417.37624454689546708L,
         38129.5594484818471461L,
         113057.05869159631492L,
         192104.047790227984431L,
         180329.498380501819718L,
         86722.3403467334749201L,
         18455.4124737722049515L,
         1229.20784182403048905L,
         -0.776491285282330997549L
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
T expint_1_rational(const T& z, const mpl::int_<64>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     3.807e-20
      // Expected Error Term:                         3.807e-20
      // Max error found at long double precision:    6.249e-20

      static const T Y = 0.66373538970947265625F;
      static const T P[6] = {    
         0.0865197248079397956816L,
         0.0275114007037026844633L,
         -0.246594388074877139824L,
         -0.0237624819878732642231L,
         -0.00259113319641673986276L,
         0.30853660894346057053e-4L
      };
      static const T Q[7] = {    
         1L,
         0.317978365797784100273L,
         0.0393622602554758722511L,
         0.00204062029115966323229L,
         0.732512107100088047854e-5L,
         -0.202872781770207871975e-5L,
         0.52779248094603709945e-7L
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Maximum Deviation Found (interpolated):     2.220e-20
      // Max error found at long double precision:   1.346e-19
      static const T P[14] = {    
         -0.534401189080684443046e-23L,
         -0.999999999999999999905L,
         -62.1517806091379402505L,
         -1568.45688271895145277L,
         -21015.3431990874009619L,
         -164333.011755931661949L,
         -777917.270775426696103L,
         -2244188.56195255112937L,
         -3888702.98145335643429L,
         -3909822.65621952648353L,
         -2149033.9538897398457L,
         -584705.537139793925189L,
         -65815.2605361889477244L,
         -2038.82870680427258038L
      };
      static const T Q[14] = {    
         1L,
         64.1517806091379399478L,
         1690.76044393722763785L,
         24035.9534033068949426L,
         203679.998633572361706L,
         1074661.58459976978285L,
         3586552.65020899358773L,
         7552186.84989547621411L,
         9853333.79353054111434L,
         7689642.74550683631258L,
         3385553.35146759180739L,
         763218.072732396428725L,
         73930.2995984054930821L,
         2063.86994219629165937L
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
T expint_1_rational(const T& z, const mpl::int_<113>&)
{
   BOOST_MATH_STD_USING
   T result;
   if(z <= 1)
   {
      // Maximum Deviation Found:                     2.477e-35
      // Expected Error Term:                         2.477e-35
      // Max error found at long double precision:    6.810e-35

      static const T Y = 0.66373538970947265625F;
      static const T P[10] = {    
         0.0865197248079397956434879099175975937L,
         0.0369066175910795772830865304506087759L,
         -0.24272036838415474665971599314725545L,
         -0.0502166331248948515282379137550178307L,
         -0.00768384138547489410285101483730424919L,
         -0.000612574337702109683505224915484717162L,
         -0.380207107950635046971492617061708534e-4L,
         -0.136528159460768830763009294683628406e-5L,
         -0.346839106212658259681029388908658618e-7L,
         -0.340500302777838063940402160594523429e-9L
      };
      static const T Q[10] = {    
         1L,
         0.426568827778942588160423015589537302L,
         0.0841384046470893490592450881447510148L,
         0.0100557215850668029618957359471132995L,
         0.000799334870474627021737357294799839363L,
         0.434452090903862735242423068552687688e-4L,
         0.15829674748799079874182885081231252e-5L,
         0.354406206738023762100882270033082198e-7L,
         0.369373328141051577845488477377890236e-9L,
         -0.274149801370933606409282434677600112e-12L
      };
      result = tools::evaluate_polynomial(P, z) 
         / tools::evaluate_polynomial(Q, z);
      result += z - log(z) - Y;
   }
   else if(z <= 4)
   {
      // Max error in interpolated form:             5.614e-35
      // Max error found at long double precision:   7.979e-35

      static const T Y = 0.70190334320068359375F;

      static const T P[17] = {    
         0.298096656795020369955077350585959794L,
         12.9314045995266142913135497455971247L,
         226.144334921582637462526628217345501L,
         2070.83670924261732722117682067381405L,
         10715.1115684330959908244769731347186L,
         30728.7876355542048019664777316053311L,
         38520.6078609349855436936232610875297L,
         -27606.0780981527583168728339620565165L,
         -169026.485055785605958655247592604835L,
         -254361.919204983608659069868035092282L,
         -195765.706874132267953259272028679935L,
         -83352.6826013533205474990119962408675L,
         -19251.6828496869586415162597993050194L,
         -2226.64251774578542836725386936102339L,
         -109.009437301400845902228611986479816L,
         -1.51492042209561411434644938098833499L
      };
      static const T Q[16] = {    
         1L,
         46.734521442032505570517810766704587L,
         908.694714348462269000247450058595655L,
         9701.76053033673927362784882748513195L,
         63254.2815292641314236625196594947774L,
         265115.641285880437335106541757711092L,
         732707.841188071900498536533086567735L,
         1348514.02492635723327306628712057794L,
         1649986.81455283047769673308781585991L,
         1326000.828522976970116271208812099L,
         683643.09490612171772350481773951341L,
         217640.505137263607952365685653352229L,
         40288.3467237411710881822569476155485L,
         3932.89353979531632559232883283175754L,
         169.845369689596739824177412096477219L,
         2.17607292280092201170768401876895354L
      };
      T recip = 1 / z;
      result = Y + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else if(z < -boost::math::tools::log_min_value<T>())
   {
      // Max error in interpolated form:             4.413e-35
      // Max error found at long double precision:   8.928e-35

      static const T P[19] = {    
         -0.559148411832951463689610809550083986e-40L,
         -0.999999999999999999999999999999999997L,
         -166.542326331163836642960118190147367L,
         -12204.639128796330005065904675153652L,
         -520807.069767086071806275022036146855L,
         -14435981.5242137970691490903863125326L,
         -274574945.737064301247496460758654196L,
         -3691611582.99810039356254671781473079L,
         -35622515944.8255047299363690814678763L,
         -248040014774.502043161750715548451142L,
         -1243190389769.53458416330946622607913L,
         -4441730126135.54739052731990368425339L,
         -11117043181899.7388524310281751971366L,
         -18976497615396.9717776601813519498961L,
         -21237496819711.1011661104761906067131L,
         -14695899122092.5161620333466757812848L,
         -5737221535080.30569711574295785864903L,
         -1077042281708.42654526404581272546244L,
         -68028222642.1941480871395695677675137L
      };
      static const T Q[20] = {    
         1L,
         168.542326331163836642960118190147311L,
         12535.7237814586576783518249115343619L,
         544891.263372016404143120911148640627L,
         15454474.7241010258634446523045237762L,
         302495899.896629522673410325891717381L,
         4215565948.38886507646911672693270307L,
         42552409471.7951815668506556705733344L,
         313592377066.753173979584098301610186L,
         1688763640223.4541980740597514904542L,
         6610992294901.59589748057620192145704L,
         18601637235659.6059890851321772682606L,
         36944278231087.2571020964163402941583L,
         50425858518481.7497071917028793820058L,
         45508060902865.0899967797848815980644L,
         25649955002765.3817331501988304758142L,
         8259575619094.6518520988612711292331L,
         1299981487496.12607474362723586264515L,
         70242279152.8241187845178443118302693L,
         -37633302.9409263839042721539363416685L
      };
      T recip = 1 / z;
      result = 1 + tools::evaluate_polynomial(P, recip)
         / tools::evaluate_polynomial(Q, recip);
      result *= exp(-z) * recip;
   }
   else
   {
      result = 0;
   }
   return result;
}

template <class T>
struct expint_fraction
{
   typedef std::pair<T,T> result_type;
   expint_fraction(unsigned n_, T z_) : b(n_ + z_), i(-1), n(n_){}
   std::pair<T,T> operator()()
   {
      std::pair<T,T> result = std::make_pair(-static_cast<T>((i+1) * (n+i)), b);
      b += 2;
      ++i;
      return result;
   }
private:
   T b;
   int i;
   unsigned n;
};

template <class T, class Policy>
inline T expint_as_fraction(unsigned n, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   BOOST_MATH_INSTRUMENT_VARIABLE(z)
   boost::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   expint_fraction<T> f(n, z);
   T result = tools::continued_fraction_b(
      f, 
      boost::math::policies::get_epsilon<T, Policy>(),
      max_iter);
   policies::check_series_iterations("boost::math::expint_continued_fraction<%1%>(unsigned,%1%)", max_iter, pol);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   BOOST_MATH_INSTRUMENT_VARIABLE(max_iter)
   result = exp(-z) / result;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   return result;
}

template <class T>
struct expint_series
{
   typedef T result_type;
   expint_series(unsigned k_, T z_, T x_k_, T denom_, T fact_) 
      : k(k_), z(z_), x_k(x_k_), denom(denom_), fact(fact_){}
   T operator()()
   {
      x_k *= -z;
      denom += 1;
      fact *= ++k;
      return x_k / (denom * fact);
   }
private:
   unsigned k;
   T z;
   T x_k;
   T denom;
   T fact;
};

template <class T, class Policy>
inline T expint_as_series(unsigned n, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   boost::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();

   BOOST_MATH_INSTRUMENT_VARIABLE(z)

   T result = 0;
   T x_k = -1;
   T denom = T(1) - n;
   T fact = 1;
   unsigned k = 0;
   for(; k < n - 1;)
   {
      result += x_k / (denom * fact);
      denom += 1;
      x_k *= -z;
      fact *= ++k;
   }
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   result += pow(-z, static_cast<T>(n - 1)) 
      * (boost::math::digamma(static_cast<T>(n)) - log(z)) / fact;
   BOOST_MATH_INSTRUMENT_VARIABLE(result)

   expint_series<T> s(k, z, x_k, denom, fact);
   result = tools::sum_series(s, policies::get_epsilon<T, Policy>(), max_iter, result);
   policies::check_series_iterations("boost::math::expint_series<%1%>(unsigned,%1%)", max_iter, pol);
   BOOST_MATH_INSTRUMENT_VARIABLE(result)
   BOOST_MATH_INSTRUMENT_VARIABLE(max_iter)
   return result;
}

template <class T, class Policy, class Tag>
T expint_imp(unsigned n, T z, const Policy& pol, const Tag& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(unsigned, %1%)";
   if(z < 0)
      return policies::raise_domain_error<T>(function, "Function requires z >= 0 but got %1%.", z, pol);
   if(z == 0)
      return n == 1 ? policies::raise_overflow_error<T>(function, 0, pol) : T(1 / (static_cast<T>(n - 1)));

   T result;

   bool f;
   if(n < 3)
   {
      f = z < 0.5;
   }
   else
   {
      f = z < (static_cast<T>(n - 2) / static_cast<T>(n - 1));
   }
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable:4127) // conditional expression is constant
#endif
   if(n == 0)
      result = exp(-z) / z;
   else if((n == 1) && (Tag::value))
   {
      result = expint_1_rational(z, tag);
   }
   else if(f)
      result = expint_as_series(n, z, pol);
   else
      result = expint_as_fraction(n, z, pol);
#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif

   return result;
}

template <class T>
struct expint_i_series
{
   typedef T result_type;
   expint_i_series(T z_) : k(0), z_k(1), z(z_){}
   T operator()()
   {
      z_k *= z / ++k;
      return z_k / k;
   }
private:
   unsigned k;
   T z_k;
   T z;
};

template <class T, class Policy>
T expint_i_as_series(T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   T result = log(z); // (log(z) - log(1 / z)) / 2;
   result += constants::euler<T>();
   expint_i_series<T> s(z);
   boost::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   result = tools::sum_series(s, policies::get_epsilon<T, Policy>(), max_iter, result);
   policies::check_series_iterations("boost::math::expint_i_series<%1%>(%1%)", max_iter, pol);
   return result;
}

template <class T, class Policy, class Tag>
T expint_i_imp(T z, const Policy& pol, const Tag& tag)
{
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, T(-z), pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);
   return expint_i_as_series(z, pol);
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const mpl::int_<53>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, -z, pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      // Maximum Deviation Found:                     2.852e-18
      // Expected Error Term:                         2.852e-18
      // Max Error found at double precision =        Poly: 2.636335e-16   Cheb: 4.187027e-16
      static const T P[10] = {    
         2.98677224343598593013L,
         0.356343618769377415068L,
         0.780836076283730801839L,
         0.114670926327032002811L,
         0.0499434773576515260534L,
         0.00726224593341228159561L,
         0.00115478237227804306827L,
         0.000116419523609765200999L,
         0.798296365679269702435e-5L,
         0.2777056254402008721e-6L
      };
      static const T Q[8] = {    
         1L,
         -1.17090412365413911947L,
         0.62215109846016746276L,
         -0.195114782069495403315L,
         0.0391523431392967238166L,
         -0.00504800158663705747345L,
         0.000389034007436065401822L,
         -0.138972589601781706598e-4L
      };

      static const T r1 = static_cast<T>(1677624236387711.0L / 4503599627370496.0L);
      static const T r2 = 0.131401834143860282009280387409357165515556574352422001206362e-16L;
      static const T r = static_cast<T>(0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392L);
      T t = (z / 3) - 1;
      result = tools::evaluate_polynomial(P, t) 
         / tools::evaluate_polynomial(Q, t);
      t = (z - r1) - r2;
      result *= t;
      if(fabs(t) < 0.1)
      {
         result += boost::math::log1p(t / r);
      }
      else
      {
         result += log(z / r);
      }
   }
   else if (z <= 10)
   {
      // Maximum Deviation Found:                     6.546e-17
      // Expected Error Term:                         6.546e-17
      // Max Error found at double precision =        Poly: 6.890169e-17   Cheb: 6.772128e-17
      static const T Y = 1.158985137939453125F;
      static const T P[8] = {    
         0.00139324086199402804173L,
         -0.0349921221823888744966L,
         -0.0264095520754134848538L,
         -0.00761224003005476438412L,
         -0.00247496209592143627977L,
         -0.000374885917942100256775L,
         -0.554086272024881826253e-4L,
         -0.396487648924804510056e-5L
      };
      static const T Q[8] = {    
         1L,
         0.744625566823272107711L,
         0.329061095011767059236L,
         0.100128624977313872323L,
         0.0223851099128506347278L,
         0.00365334190742316650106L,
         0.000402453408512476836472L,
         0.263649630720255691787e-4L
      };
      T t = z / 2 - 4;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 20)
   {
      // Maximum Deviation Found:                     1.843e-17
      // Expected Error Term:                         -1.842e-17
      // Max Error found at double precision =        Poly: 4.375868e-17   Cheb: 5.860967e-17

      static const T Y = 1.0869731903076171875F;
      static const T P[9] = {    
         -0.00893891094356945667451L,
         -0.0484607730127134045806L,
         -0.0652810444222236895772L,
         -0.0478447572647309671455L,
         -0.0226059218923777094596L,
         -0.00720603636917482065907L,
         -0.00155941947035972031334L,
         -0.000209750022660200888349L,
         -0.138652200349182596186e-4L
      };
      static const T Q[9] = {    
         1L,
         1.97017214039061194971L,
         1.86232465043073157508L,
         1.09601437090337519977L,
         0.438873285773088870812L,
         0.122537731979686102756L,
         0.0233458478275769288159L,
         0.00278170769163303669021L,
         0.000159150281166108755531L
      };
      T t = z / 5 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 40)
   {
      // Maximum Deviation Found:                     5.102e-18
      // Expected Error Term:                         5.101e-18
      // Max Error found at double precision =        Poly: 1.441088e-16   Cheb: 1.864792e-16


      static const T Y = 1.03937530517578125F;
      static const T P[9] = {    
         -0.00356165148914447597995L,
         -0.0229930320357982333406L,
         -0.0449814350482277917716L,
         -0.0453759383048193402336L,
         -0.0272050837209380717069L,
         -0.00994403059883350813295L,
         -0.00207592267812291726961L,
         -0.000192178045857733706044L,
         -0.113161784705911400295e-9L
      };
      static const T Q[9] = {    
         1L,
         2.84354408840148561131L,
         3.6599610090072393012L,
         2.75088464344293083595L,
         1.2985244073998398643L,
         0.383213198510794507409L,
         0.0651165455496281337831L,
         0.00488071077519227853585L
      };
      T t = z / 10 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else
   {
      // Max Error found at double precision =        3.381886e-17
      static const T exp40 = static_cast<T>(2.35385266837019985407899910749034804508871617254555467236651e17L);
      static const T Y= 1.013065338134765625F;
      static const T P[6] = {    
         -0.0130653381347656243849L,
         0.19029710559486576682L,
         94.7365094537197236011L,
         -2516.35323679844256203L,
         18932.0850014925993025L,
         -38703.1431362056714134L
      };
      static const T Q[7] = {    
         1L,
         61.9733592849439884145L,
         -2354.56211323420194283L,
         22329.1459489893079041L,
         -70126.245140396567133L,
         54738.2833147775537106L,
         8297.16296356518409347L
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const mpl::int_<64>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, -z, pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      // Maximum Deviation Found:                     3.883e-21
      // Expected Error Term:                         3.883e-21
      // Max Error found at long double precision =   Poly: 3.344801e-19   Cheb: 4.989937e-19

      static const T P[11] = {    
         2.98677224343598593764L,
         0.25891613550886736592L,
         0.789323584998672832285L,
         0.092432587824602399339L,
         0.0514236978728625906656L,
         0.00658477469745132977921L,
         0.00124914538197086254233L,
         0.000131429679565472408551L,
         0.11293331317982763165e-4L,
         0.629499283139417444244e-6L,
         0.177833045143692498221e-7L
      };
      static const T Q[9] = {    
         1L,
         -1.20352377969742325748L,
         0.66707904942606479811L,
         -0.223014531629140771914L,
         0.0493340022262908008636L,
         -0.00741934273050807310677L,
         0.00074353567782087939294L,
         -0.455861727069603367656e-4L,
         0.131515429329812837701e-5L
      };

      static const T r1 = static_cast<T>(1677624236387711.0L / 4503599627370496.0L);
      static const T r2 = 0.131401834143860282009280387409357165515556574352422001206362e-16L;
      static const T r = static_cast<T>(0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392L);
      T t = (z / 3) - 1;
      result = tools::evaluate_polynomial(P, t) 
         / tools::evaluate_polynomial(Q, t);
      t = (z - r1) - r2;
      result *= t;
      if(fabs(t) < 0.1)
      {
         result += boost::math::log1p(t / r);
      }
      else
      {
         result += log(z / r);
      }
   }
   else if (z <= 10)
   {
      // Maximum Deviation Found:                     2.622e-21
      // Expected Error Term:                         -2.622e-21
      // Max Error found at long double precision =   Poly: 1.208328e-20   Cheb: 1.073723e-20

      static const T Y = 1.158985137939453125F;
      static const T P[9] = {    
         0.00139324086199409049399L,
         -0.0345238388952337563247L,
         -0.0382065278072592940767L,
         -0.0156117003070560727392L,
         -0.00383276012430495387102L,
         -0.000697070540945496497992L,
         -0.877310384591205930343e-4L,
         -0.623067256376494930067e-5L,
         -0.377246883283337141444e-6L
      };
      static const T Q[10] = {    
         1L,
         1.08073635708902053767L,
         0.553681133533942532909L,
         0.176763647137553797451L,
         0.0387891748253869928121L,
         0.0060603004848394727017L,
         0.000670519492939992806051L,
         0.4947357050100855646e-4L,
         0.204339282037446434827e-5L,
         0.146951181174930425744e-7L
      };
      T t = z / 2 - 4;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 20)
   {
      // Maximum Deviation Found:                     3.220e-20
      // Expected Error Term:                         3.220e-20
      // Max Error found at long double precision =   Poly: 7.696841e-20   Cheb: 6.205163e-20


      static const T Y = 1.0869731903076171875F;
      static const T P[10] = {    
         -0.00893891094356946995368L,
         -0.0487562980088748775943L,
         -0.0670568657950041926085L,
         -0.0509577352851442932713L,
         -0.02551800927409034206L,
         -0.00892913759760086687083L,
         -0.00224469630207344379888L,
         -0.000392477245911296982776L,
         -0.44424044184395578775e-4L,
         -0.252788029251437017959e-5L
      };
      static const T Q[10] = {    
         1L,
         2.00323265503572414261L,
         1.94688958187256383178L,
         1.19733638134417472296L,
         0.513137726038353385661L,
         0.159135395578007264547L,
         0.0358233587351620919881L,
         0.0056716655597009417875L,
         0.000577048986213535829925L,
         0.290976943033493216793e-4L
      };
      T t = z / 5 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 40)
   {
      // Maximum Deviation Found:                     2.940e-21
      // Expected Error Term:                         -2.938e-21
      // Max Error found at long double precision =   Poly: 3.419893e-19   Cheb: 3.359874e-19

      static const T Y = 1.03937530517578125F;
      static const T P[12] = {    
         -0.00356165148914447278177L,
         -0.0240235006148610849678L,
         -0.0516699967278057976119L,
         -0.0586603078706856245674L,
         -0.0409960120868776180825L,
         -0.0185485073689590665153L,
         -0.00537842101034123222417L,
         -0.000920988084778273760609L,
         -0.716742618812210980263e-4L,
         -0.504623302166487346677e-9L,
         0.712662196671896837736e-10L,
         -0.533769629702262072175e-11L
      };
      static const T Q[9] = {    
         1L,
         3.13286733695729715455L,
         4.49281223045653491929L,
         3.84900294427622911374L,
         2.15205199043580378211L,
         0.802912186540269232424L,
         0.194793170017818925388L,
         0.0280128013584653182994L,
         0.00182034930799902922549L
      };
      T t = z / 10 - 3;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else
   {
      // Maximum Deviation Found:                     3.536e-20
      // Max Error found at long double precision =   Poly: 1.310671e-19   Cheb: 8.630943e-11

      static const T exp40 = static_cast<T>(2.35385266837019985407899910749034804508871617254555467236651e17L);
      static const T Y= 1.013065338134765625F;
      static const T P[9] = {    
         -0.0130653381347656250004L,
         0.644487780349757303739L,
         143.995670348227433964L,
         -13918.9322758014173709L,
         476260.975133624194484L,
         -7437102.15135982802122L,
         53732298.8764767916542L,
         -160695051.957997452509L,
         137839271.592778020028L
      };
      static const T Q[9] = {    
         1L,
         27.2103343964943718802L,
         -8785.48528692879413676L,
         397530.290000322626766L,
         -7356441.34957799368252L,
         63050914.5343400957524L,
         -246143779.638307701369L,
         384647824.678554961174L,
         -166288297.874583961493L
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy>
T expint_i_imp(T z, const Policy& pol, const mpl::int_<113>& tag)
{
   BOOST_MATH_STD_USING
   static const char* function = "boost::math::expint<%1%>(%1%)";
   if(z < 0)
      return -expint_imp(1, -z, pol, tag);
   if(z == 0)
      return -policies::raise_overflow_error<T>(function, 0, pol);

   T result;

   if(z <= 6)
   {
      // Maximum Deviation Found:                     1.230e-36
      // Expected Error Term:                         -1.230e-36
      // Max Error found at long double precision =   Poly: 4.355299e-34   Cheb: 7.512581e-34


      static const T P[15] = {    
         2.98677224343598593765287235997328555L,
         -0.333256034674702967028780537349334037L,
         0.851831522798101228384971644036708463L,
         -0.0657854833494646206186773614110374948L,
         0.0630065662557284456000060708977935073L,
         -0.00311759191425309373327784154659649232L,
         0.00176213568201493949664478471656026771L,
         -0.491548660404172089488535218163952295e-4L,
         0.207764227621061706075562107748176592e-4L,
         -0.225445398156913584846374273379402765e-6L,
         0.996939977231410319761273881672601592e-7L,
         0.212546902052178643330520878928100847e-9L,
         0.154646053060262871360159325115980023e-9L,
         0.143971277122049197323415503594302307e-11L,
         0.306243138978114692252817805327426657e-13L
      };
      static const T Q[15] = {    
         1L,
         -1.40178870313943798705491944989231793L,
         0.943810968269701047641218856758605284L,
         -0.405026631534345064600850391026113165L,
         0.123924153524614086482627660399122762L,
         -0.0286364505373369439591132549624317707L,
         0.00516148845910606985396596845494015963L,
         -0.000738330799456364820380739850924783649L,
         0.843737760991856114061953265870882637e-4L,
         -0.767957673431982543213661388914587589e-5L,
         0.549136847313854595809952100614840031e-6L,
         -0.299801381513743676764008325949325404e-7L,
         0.118419479055346106118129130945423483e-8L,
         -0.30372295663095470359211949045344607e-10L,
         0.382742953753485333207877784720070523e-12L
      };

      static const T r1 = static_cast<T>(1677624236387711.0L / 4503599627370496.0L);
      static const T r2 = static_cast<T>(266514582277687.0L / 4503599627370496.0L / 4503599627370496.0L);
      static const T r3 = static_cast<T>(0.283806480836357377069325311780969887585024578164571984232357e-31L);
      static const T r = static_cast<T>(0.372507410781366634461991866580119133535689497771654051555657435242200120636201854384926049951548942392L);
      T t = (z / 3) - 1;
      result = tools::evaluate_polynomial(P, t) 
         / tools::evaluate_polynomial(Q, t);
      t = ((z - r1) - r2) - r3;
      result *= t;
      if(fabs(t) < 0.1)
      {
         result += boost::math::log1p(t / r);
      }
      else
      {
         result += log(z / r);
      }
   }
   else if (z <= 10)
   {
      // Maximum Deviation Found:                     7.779e-36
      // Expected Error Term:                         -7.779e-36
      // Max Error found at long double precision =   Poly: 2.576723e-35   Cheb: 1.236001e-34

      static const T Y = 1.158985137939453125F;
      static const T P[15] = {    
         0.00139324086199409049282472239613554817L,
         -0.0338173111691991289178779840307998955L,
         -0.0555972290794371306259684845277620556L,
         -0.0378677976003456171563136909186202177L,
         -0.0152221583517528358782902783914356667L,
         -0.00428283334203873035104248217403126905L,
         -0.000922782631491644846511553601323435286L,
         -0.000155513428088853161562660696055496696L,
         -0.205756580255359882813545261519317096e-4L,
         -0.220327406578552089820753181821115181e-5L,
         -0.189483157545587592043421445645377439e-6L,
         -0.122426571518570587750898968123803867e-7L,
         -0.635187358949437991465353268374523944e-9L,
         -0.203015132965870311935118337194860863e-10L,
         -0.384276705503357655108096065452950822e-12L
      };
      static const T Q[15] = {    
         1L,
         1.58784732785354597996617046880946257L,
         1.18550755302279446339364262338114098L,
         0.55598993549661368604527040349702836L,
         0.184290888380564236919107835030984453L,
         0.0459658051803613282360464632326866113L,
         0.0089505064268613225167835599456014705L,
         0.00139042673882987693424772855926289077L,
         0.000174210708041584097450805790176479012L,
         0.176324034009707558089086875136647376e-4L,
         0.142935845999505649273084545313710581e-5L,
         0.907502324487057260675816233312747784e-7L,
         0.431044337808893270797934621235918418e-8L,
         0.139007266881450521776529705677086902e-9L,
         0.234715286125516430792452741830364672e-11L
      };
      T t = z / 2 - 4;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 18)
   {
      // Maximum Deviation Found:                     1.082e-34
      // Expected Error Term:                         1.080e-34
      // Max Error found at long double precision =   Poly: 1.958294e-34   Cheb: 2.472261e-34


      static const T Y = 1.091579437255859375F;
      static const T P[17] = {    
         -0.00685089599550151282724924894258520532L,
         -0.0443313550253580053324487059748497467L,
         -0.071538561252424027443296958795814874L,
         -0.0622923153354102682285444067843300583L,
         -0.0361631270264607478205393775461208794L,
         -0.0153192826839624850298106509601033261L,
         -0.00496967904961260031539602977748408242L,
         -0.00126989079663425780800919171538920589L,
         -0.000258933143097125199914724875206326698L,
         -0.422110326689204794443002330541441956e-4L,
         -0.546004547590412661451073996127115221e-5L,
         -0.546775260262202177131068692199272241e-6L,
         -0.404157632825805803833379568956559215e-7L,
         -0.200612596196561323832327013027419284e-8L,
         -0.502538501472133913417609379765434153e-10L,
         -0.326283053716799774936661568391296584e-13L,
         0.869226483473172853557775877908693647e-15L
      };
      static const T Q[15] = {    
         1L,
         2.23227220874479061894038229141871087L,
         2.40221000361027971895657505660959863L,
         1.65476320985936174728238416007084214L,
         0.816828602963895720369875535001248227L,
         0.306337922909446903672123418670921066L,
         0.0902400121654409267774593230720600752L,
         0.0212708882169429206498765100993228086L,
         0.00404442626252467471957713495828165491L,
         0.0006195601618842253612635241404054589L,
         0.755930932686543009521454653994321843e-4L,
         0.716004532773778954193609582677482803e-5L,
         0.500881663076471627699290821742924233e-6L,
         0.233593219218823384508105943657387644e-7L,
         0.554900353169148897444104962034267682e-9L
      };
      T t = z / 4 - 3.5;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else if(z <= 26)
   {
      // Maximum Deviation Found:                     3.163e-35
      // Expected Error Term:                         3.163e-35
      // Max Error found at long double precision =   Poly: 4.158110e-35   Cheb: 5.385532e-35

      static const T Y = 1.051731109619140625F;
      static const T P[14] = {    
         -0.00144552494420652573815404828020593565L,
         -0.0126747451594545338365684731262912741L,
         -0.01757394877502366717526779263438073L,
         -0.0126838952395506921945756139424722588L,
         -0.0060045057928894974954756789352443522L,
         -0.00205349237147226126653803455793107903L,
         -0.000532606040579654887676082220195624207L,
         -0.000107344687098019891474772069139014662L,
         -0.169536802705805811859089949943435152e-4L,
         -0.20863311729206543881826553010120078e-5L,
         -0.195670358542116256713560296776654385e-6L,
         -0.133291168587253145439184028259772437e-7L,
         -0.595500337089495614285777067722823397e-9L,
         -0.133141358866324100955927979606981328e-10L
      };
      static const T Q[14] = {    
         1L,
         1.72490783907582654629537013560044682L,
         1.44524329516800613088375685659759765L,
         0.778241785539308257585068744978050181L,
         0.300520486589206605184097270225725584L,
         0.0879346899691339661394537806057953957L,
         0.0200802415843802892793583043470125006L,
         0.00362842049172586254520256100538273214L,
         0.000519731362862955132062751246769469957L,
         0.584092147914050999895178697392282665e-4L,
         0.501851497707855358002773398333542337e-5L,
         0.313085677467921096644895738538865537e-6L,
         0.127552010539733113371132321521204458e-7L,
         0.25737310826983451144405899970774587e-9L
      };
      T t = z / 4 - 5.5;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else if(z <= 42)
   {
      // Maximum Deviation Found:                     7.972e-36
      // Expected Error Term:                         7.962e-36
      // Max Error found at long double precision =   Poly: 1.711721e-34   Cheb: 3.100018e-34

      static const T Y = 1.032726287841796875F;
      static const T P[15] = {    
         -0.00141056919297307534690895009969373233L,
         -0.0123384175302540291339020257071411437L,
         -0.0298127270706864057791526083667396115L,
         -0.0390686759471630584626293670260768098L,
         -0.0338226792912607409822059922949035589L,
         -0.0211659736179834946452561197559654582L,
         -0.0100428887460879377373158821400070313L,
         -0.00370717396015165148484022792801682932L,
         -0.0010768667551001624764329000496561659L,
         -0.000246127328761027039347584096573123531L,
         -0.437318110527818613580613051861991198e-4L,
         -0.587532682329299591501065482317771497e-5L,
         -0.565697065670893984610852937110819467e-6L,
         -0.350233957364028523971768887437839573e-7L,
         -0.105428907085424234504608142258423505e-8L
      };
      static const T Q[16] = {    
         1L,
         3.17261315255467581204685605414005525L,
         4.85267952971640525245338392887217426L,
         4.74341914912439861451492872946725151L,
         3.31108463283559911602405970817931801L,
         1.74657006336994649386607925179848899L,
         0.718255607416072737965933040353653244L,
         0.234037553177354542791975767960643864L,
         0.0607470145906491602476833515412605389L,
         0.0125048143774226921434854172947548724L,
         0.00201034366420433762935768458656609163L,
         0.000244823338417452367656368849303165721L,
         0.213511655166983177960471085462540807e-4L,
         0.119323998465870686327170541547982932e-5L,
         0.322153582559488797803027773591727565e-7L,
         -0.161635525318683508633792845159942312e-16L
      };
      T t = z / 8 - 4.25;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else if(z <= 56)
   {
      // Maximum Deviation Found:                     4.469e-36
      // Expected Error Term:                         4.468e-36
      // Max Error found at long double precision =   Poly: 1.288958e-35   Cheb: 2.304586e-35

      static const T Y = 1.0216197967529296875F;
      static const T P[12] = {    
         -0.000322999116096627043476023926572650045L,
         -0.00385606067447365187909164609294113346L,
         -0.00686514524727568176735949971985244415L,
         -0.00606260649593050194602676772589601799L,
         -0.00334382362017147544335054575436194357L,
         -0.00126108534260253075708625583630318043L,
         -0.000337881489347846058951220431209276776L,
         -0.648480902304640018785370650254018022e-4L,
         -0.87652644082970492211455290209092766e-5L,
         -0.794712243338068631557849449519994144e-6L,
         -0.434084023639508143975983454830954835e-7L,
         -0.107839681938752337160494412638656696e-8L
      };
      static const T Q[12] = {    
         1L,
         2.09913805456661084097134805151524958L,
         2.07041755535439919593503171320431849L,
         1.26406517226052371320416108604874734L,
         0.529689923703770353961553223973435569L,
         0.159578150879536711042269658656115746L,
         0.0351720877642000691155202082629857131L,
         0.00565313621289648752407123620997063122L,
         0.000646920278540515480093843570291218295L,
         0.499904084850091676776993523323213591e-4L,
         0.233740058688179614344680531486267142e-5L,
         0.498800627828842754845418576305379469e-7L
      };
      T t = z / 7 - 7;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else if(z <= 84)
   {
      // Maximum Deviation Found:                     5.588e-35
      // Expected Error Term:                         -5.566e-35
      // Max Error found at long double precision =   Poly: 9.976345e-35   Cheb: 8.358865e-35

      static const T Y = 1.015148162841796875F;
      static const T P[11] = {    
         -0.000435714784725086961464589957142615216L,
         -0.00432114324353830636009453048419094314L,
         -0.0100740363285526177522819204820582424L,
         -0.0116744115827059174392383504427640362L,
         -0.00816145387784261141360062395898644652L,
         -0.00371380272673500791322744465394211508L,
         -0.00112958263488611536502153195005736563L,
         -0.000228316462389404645183269923754256664L,
         -0.29462181955852860250359064291292577e-4L,
         -0.21972450610957417963227028788460299e-5L,
         -0.720558173805289167524715527536874694e-7L
      };
      static const T Q[11] = {    
         1L,
         2.95918362458402597039366979529287095L,
         3.96472247520659077944638411856748924L,
         3.15563251550528513747923714884142131L,
         1.64674612007093983894215359287448334L,
         0.58695020129846594405856226787156424L,
         0.144358385319329396231755457772362793L,
         0.024146911506411684815134916238348063L,
         0.0026257132337460784266874572001650153L,
         0.000167479843750859222348869769094711093L,
         0.475673638665358075556452220192497036e-5L
      };
      T t = z / 14 - 5;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result *= exp(z) / z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
      result += z;
      BOOST_MATH_INSTRUMENT_VARIABLE(result)
   }
   else if(z <= 210)
   {
      // Maximum Deviation Found:                     4.448e-36
      // Expected Error Term:                         4.445e-36
      // Max Error found at long double precision =   Poly: 2.058532e-35   Cheb: 2.165465e-27

      static const T Y= 1.00849151611328125F;
      static const T P[9] = {    
         -0.0084915161132812500000001440233607358L,
         1.84479378737716028341394223076147872L,
         -130.431146923726715674081563022115568L,
         4336.26945491571504885214176203512015L,
         -76279.0031974974730095170437591004177L,
         729577.956271997673695191455111727774L,
         -3661928.69330208734947103004900349266L,
         8570600.041606912735872059184527855L,
         -6758379.93672362080947905580906028645L
      };
      static const T Q[10] = {    
         1L,
         -99.4868026047611434569541483506091713L,
         3879.67753690517114249705089803055473L,
         -76495.82413252517165830203774900806L,
         820773.726408311894342553758526282667L,
         -4803087.64956923577571031564909646579L,
         14521246.227703545012713173740895477L,
         -19762752.0196769712258527849159393044L,
         8354144.67882768405803322344185185517L,
         355076.853106511136734454134915432571L
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      result *= exp(z) / z;
      result += z;
   }
   else // z > 210
   {
      // Maximum Deviation Found:                     3.963e-37
      // Expected Error Term:                         3.963e-37
      // Max Error found at long double precision =   Poly: 1.248049e-36   Cheb: 2.843486e-29

      static const T exp40 = static_cast<T>(2.35385266837019985407899910749034804508871617254555467236651e17L);
      static const T Y= 1.00252532958984375F;
      static const T P[8] = {    
         -0.00252532958984375000000000000000000085L,
         1.16591386866059087390621952073890359L,
         -67.8483431314018462417456828499277579L,
         1567.68688154683822956359536287575892L,
         -17335.4683325819116482498725687644986L,
         93632.6567462673524739954389166550069L,
         -225025.189335919133214440347510936787L,
         175864.614717440010942804684741336853L
      };
      static const T Q[9] = {    
         1L,
         -65.6998869881600212224652719706425129L,
         1642.73850032324014781607859416890077L,
         -19937.2610222467322481947237312818575L,
         124136.267326632742667972126625064538L,
         -384614.251466704550678760562965502293L,
         523355.035910385688578278384032026998L,
         -217809.552260834025885677791936351294L,
         -8555.81719551123640677261226549550872L
      };
      T t = 1 / z;
      result = Y + tools::evaluate_polynomial(P, t)
         / tools::evaluate_polynomial(Q, t);
      if(z < 41)
         result *= exp(z) / z;
      else
      {
         // Avoid premature overflow if we can:
         t = z - 40;
         if(t > tools::log_max_value<T>())
         {
            result = policies::raise_overflow_error<T>(function, 0, pol);
         }
         else
         {
            result *= exp(z - 40) / z;
            if(result > tools::max_value<T>() / exp40)
            {
               result = policies::raise_overflow_error<T>(function, 0, pol);
            }
            else
            {
               result *= exp40;
            }
         }
      }
      result += z;
   }
   return result;
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint_forwarder(T z, const Policy& /*pol*/, mpl::true_ const&)
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

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::expint_i_imp(
      static_cast<value_type>(z),
      forwarding_policy(),
      tag_type()), "boost::math::expint<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type
expint_forwarder(unsigned n, T z, const mpl::false_&)
{
   return boost::math::expint(n, z, policies::policy<>());
}

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type
   expint(unsigned n, T z, const Policy& /*pol*/)
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

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::expint_imp(
      n,
      static_cast<value_type>(z),
      forwarding_policy(),
      tag_type()), "boost::math::expint<%1%>(unsigned, %1%)");
}

template <class T, class U>
inline typename detail::expint_result<T, U>::type
   expint(T const z, U const u)
{
   typedef typename policies::is_policy<U>::type tag_type;
   return detail::expint_forwarder(z, u, tag_type());
}

template <class T>
inline typename tools::promote_args<T>::type
   expint(T z)
{
   return expint(z, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_EXPINT_HPP


