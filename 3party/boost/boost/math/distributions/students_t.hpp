//  Copyright John Maddock 2006.
//  Copyright Paul A. Bristow 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_STUDENTS_T_HPP
#define BOOST_STATS_STUDENTS_T_HPP

// http://en.wikipedia.org/wiki/Student%27s_t_distribution
// http://www.itl.nist.gov/div898/handbook/eda/section3/eda3664.htm

#include <boost/math/distributions/fwd.hpp>
#include <boost/math/special_functions/beta.hpp> // for ibeta(a, b, x).
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>

#include <utility>

#ifdef BOOST_MSVC
# pragma warning(push)
# pragma warning(disable: 4702) // unreachable code (return after domain_error throw).
#endif

namespace boost{ namespace math{

template <class RealType = double, class Policy = policies::policy<> >
class students_t_distribution
{
public:
   typedef RealType value_type;
   typedef Policy policy_type;

   students_t_distribution(RealType i) : m_df(i)
   { // Constructor.
      RealType result;
      detail::check_df(
         "boost::math::students_t_distribution<%1%>::students_t_distribution", m_df, &result, Policy());
   } // students_t_distribution

   RealType degrees_of_freedom()const
   {
      return m_df;
   }

   // Parameter estimation:
   static RealType find_degrees_of_freedom(
      RealType difference_from_mean,
      RealType alpha,
      RealType beta,
      RealType sd,
      RealType hint = 100);

private:
   //
   // Data members:
   //
   RealType m_df;  // degrees of freedom are a real number.
};

typedef students_t_distribution<double> students_t;

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> range(const students_t_distribution<RealType, Policy>& /*dist*/)
{ // Range of permissible values for random variable x.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>());
}

template <class RealType, class Policy>
inline const std::pair<RealType, RealType> support(const students_t_distribution<RealType, Policy>& /*dist*/)
{ // Range of supported values for random variable x.
   // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.
   using boost::math::tools::max_value;
   return std::pair<RealType, RealType>(-max_value<RealType>(), max_value<RealType>());
}

template <class RealType, class Policy>
inline RealType pdf(const students_t_distribution<RealType, Policy>& dist, const RealType& t)
{
   BOOST_FPU_EXCEPTION_GUARD
   BOOST_MATH_STD_USING  // for ADL of std functions

   RealType degrees_of_freedom = dist.degrees_of_freedom();
   // Error check:
   RealType error_result;
   if(false == detail::check_df(
      "boost::math::pdf(const students_t_distribution<%1%>&, %1%)", degrees_of_freedom, &error_result, Policy()))
      return error_result;
   // Might conceivably permit df = +infinity and use normal distribution.
   RealType result;
   RealType basem1 = t * t / degrees_of_freedom;
   if(basem1 < 0.125)
   {
      result = exp(-boost::math::log1p(basem1, Policy()) * (1+degrees_of_freedom) / 2);
   }
   else
   {
      result = pow(1 / (1 + basem1), (degrees_of_freedom + 1) / 2);
   }
   result /= sqrt(degrees_of_freedom) * boost::math::beta(degrees_of_freedom / 2, RealType(0.5f), Policy());
   return result;
} // pdf

template <class RealType, class Policy>
inline RealType cdf(const students_t_distribution<RealType, Policy>& dist, const RealType& t)
{
   RealType degrees_of_freedom = dist.degrees_of_freedom();
   // Error check:
   RealType error_result;
   if(false == detail::check_df(
      "boost::math::cdf(const students_t_distribution<%1%>&, %1%)", degrees_of_freedom, &error_result, Policy()))
      return error_result;

   if (t == 0)
   {
     return 0.5;
   }
   //
   // Calculate probability of Student's t using the incomplete beta function.
   // probability = ibeta(degrees_of_freedom / 2, 1/2, degrees_of_freedom / (degrees_of_freedom + t*t))
   //
   // However when t is small compared to the degrees of freedom, that formula
   // suffers from rounding error, use the identity formula to work around
   // the problem:
   //
   // I[x](a,b) = 1 - I[1-x](b,a)
   //
   // and:
   //
   //     x = df / (df + t^2)
   //
   // so:
   //
   // 1 - x = t^2 / (df + t^2)
   //
   RealType t2 = t * t;
   RealType probability;
   if(degrees_of_freedom > 2 * t2)
   {
      RealType z = t2 / (degrees_of_freedom + t2);
      probability = ibetac(static_cast<RealType>(0.5), degrees_of_freedom / 2, z, Policy()) / 2;
   }
   else
   {
      RealType z = degrees_of_freedom / (degrees_of_freedom + t2);
      probability = ibeta(degrees_of_freedom / 2, static_cast<RealType>(0.5), z, Policy()) / 2;
   }
   return (t > 0 ? 1   - probability : probability);
} // cdf

template <class RealType, class Policy>
inline RealType quantile(const students_t_distribution<RealType, Policy>& dist, const RealType& p)
{
   BOOST_MATH_STD_USING // for ADL of std functions
   //
   // Obtain parameters:
   //
   RealType degrees_of_freedom = dist.degrees_of_freedom();
   RealType probability = p;
   //
   // Check for domain errors:
   //
   static const char* function = "boost::math::quantile(const students_t_distribution<%1%>&, %1%)";
   RealType error_result;
   if(false == detail::check_df(
      function, degrees_of_freedom, &error_result, Policy())
         && detail::check_probability(function, probability, &error_result, Policy()))
      return error_result;

   // Special cases, regardless of degrees_of_freedom.
   if (probability == 0)
      return -policies::raise_overflow_error<RealType>(function, 0, Policy());
   if (probability == 1)
     return policies::raise_overflow_error<RealType>(function, 0, Policy());
   if (probability == static_cast<RealType>(0.5))
     return 0;
   //
   // This next block is disabled in favour of a faster method than
   // incomplete beta inverse, code retained for future reference:
   //
#if 0
   //
   // Calculate quantile of Student's t using the incomplete beta function inverse:
   //
   probability = (probability > 0.5) ? 1 - probability : probability;
   RealType t, x, y;
   x = ibeta_inv(degrees_of_freedom / 2, RealType(0.5), 2 * probability, &y);
   if(degrees_of_freedom * y > tools::max_value<RealType>() * x)
      t = tools::overflow_error<RealType>(function);
   else
      t = sqrt(degrees_of_freedom * y / x);
   //
   // Figure out sign based on the size of p:
   //
   if(p < 0.5)
      t = -t;

   return t;
#endif
   //
   // Depending on how many digits RealType has, this may forward
   // to the incomplete beta inverse as above.  Otherwise uses a
   // faster method that is accurate to ~15 digits everywhere
   // and a couple of epsilon at double precision and in the central 
   // region where most use cases will occur...
   //
   return boost::math::detail::fast_students_t_quantile(degrees_of_freedom, probability, Policy());
} // quantile

template <class RealType, class Policy>
inline RealType cdf(const complemented2_type<students_t_distribution<RealType, Policy>, RealType>& c)
{
   return cdf(c.dist, -c.param);
}

template <class RealType, class Policy>
inline RealType quantile(const complemented2_type<students_t_distribution<RealType, Policy>, RealType>& c)
{
   return -quantile(c.dist, c.param);
}

//
// Parameter estimation follows:
//
namespace detail{
//
// Functors for finding degrees of freedom:
//
template <class RealType, class Policy>
struct sample_size_func
{
   sample_size_func(RealType a, RealType b, RealType s, RealType d)
      : alpha(a), beta(b), ratio(s*s/(d*d)) {}

   RealType operator()(const RealType& df)
   {
      if(df <= tools::min_value<RealType>())
         return 1;
      students_t_distribution<RealType, Policy> t(df);
      RealType qa = quantile(complement(t, alpha));
      RealType qb = quantile(complement(t, beta));
      qa += qb;
      qa *= qa;
      qa *= ratio;
      qa -= (df + 1);
      return qa;
   }
   RealType alpha, beta, ratio;
};

}  // namespace detail

template <class RealType, class Policy>
RealType students_t_distribution<RealType, Policy>::find_degrees_of_freedom(
      RealType difference_from_mean,
      RealType alpha,
      RealType beta,
      RealType sd,
      RealType hint)
{
   static const char* function = "boost::math::students_t_distribution<%1%>::find_degrees_of_freedom";
   //
   // Check for domain errors:
   //
   RealType error_result;
   if(false == detail::check_probability(
      function, alpha, &error_result, Policy())
         && detail::check_probability(function, beta, &error_result, Policy()))
      return error_result;

   if(hint <= 0)
      hint = 1;

   detail::sample_size_func<RealType, Policy> f(alpha, beta, sd, difference_from_mean);
   tools::eps_tolerance<RealType> tol(policies::digits<RealType, Policy>());
   boost::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
   std::pair<RealType, RealType> r = tools::bracket_and_solve_root(f, hint, RealType(2), false, tol, max_iter, Policy());
   RealType result = r.first + (r.second - r.first) / 2;
   if(max_iter >= policies::get_max_root_iterations<Policy>())
   {
      policies::raise_evaluation_error<RealType>(function, "Unable to locate solution in a reasonable time:"
         " either there is no answer to how many degrees of freedom are required"
         " or the answer is infinite.  Current best guess is %1%", result, Policy());
   }
   return result;
}

template <class RealType, class Policy>
inline RealType mean(const students_t_distribution<RealType, Policy>& )
{
   return 0;
}

template <class RealType, class Policy>
inline RealType variance(const students_t_distribution<RealType, Policy>& dist)
{
   // Error check:
   RealType error_result;
   if(false == detail::check_df(
      "boost::math::variance(students_t_distribution<%1%> const&, %1%)", dist.degrees_of_freedom(), &error_result, Policy()))
      return error_result;

   RealType v = dist.degrees_of_freedom();
   return v / (v - 2);
}

template <class RealType, class Policy>
inline RealType mode(const students_t_distribution<RealType, Policy>& /*dist*/)
{
   return 0;
}

template <class RealType, class Policy>
inline RealType median(const students_t_distribution<RealType, Policy>& /*dist*/)
{
   return 0;
}

template <class RealType, class Policy>
inline RealType skewness(const students_t_distribution<RealType, Policy>& dist)
{
   if(dist.degrees_of_freedom() <= 3)
   {
      policies::raise_domain_error<RealType>(
         "boost::math::skewness(students_t_distribution<%1%> const&, %1%)",
         "Skewness is undefined for degrees of freedom <= 3, but got %1%.",
         dist.degrees_of_freedom(), Policy());
   }
   return 0;
}

template <class RealType, class Policy>
inline RealType kurtosis(const students_t_distribution<RealType, Policy>& dist)
{
   RealType df = dist.degrees_of_freedom();
   if(df <= 3)
   {
      policies::raise_domain_error<RealType>(
         "boost::math::kurtosis(students_t_distribution<%1%> const&, %1%)",
         "Skewness is undefined for degrees of freedom <= 3, but got %1%.",
         df, Policy());
   }
   return 3 * (df - 2) / (df - 4);
}

template <class RealType, class Policy>
inline RealType kurtosis_excess(const students_t_distribution<RealType, Policy>& dist)
{
   // see http://mathworld.wolfram.com/Kurtosis.html
   RealType df = dist.degrees_of_freedom();
   if(df <= 3)
   {
      policies::raise_domain_error<RealType>(
         "boost::math::kurtosis_excess(students_t_distribution<%1%> const&, %1%)",
         "Skewness is undefined for degrees of freedom <= 3, but got %1%.",
         df, Policy());
   }
   return 6 / (df - 4);
}

} // namespace math
} // namespace boost

#ifdef BOOST_MSVC
# pragma warning(pop)
#endif

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_STUDENTS_T_HPP
