//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// distributions.hpp provides definitions of the concept of a distribution
// and non-member accessor functions that must be implemented by all distributions.
// This is used to verify that
// all the features of a distributions have been fully implemented.

#ifndef BOOST_MATH_DISTRIBUTION_CONCEPT_HPP
#define BOOST_MATH_DISTRIBUTION_CONCEPT_HPP

#include <boost/math/distributions/complement.hpp>
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4100)
#pragma warning(disable: 4510)
#pragma warning(disable: 4610)
#endif
#include <boost/concept_check.hpp>
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
#include <utility>

namespace boost{
namespace math{

namespace concepts
{
// Begin by defining a concept archetype
// for a distribution class:
//
template <class RealType>
class distribution_archetype
{
public:
   typedef RealType value_type;

   distribution_archetype(const distribution_archetype&); // Copy constructible.
   distribution_archetype& operator=(const distribution_archetype&); // Assignable.

   // There is no default constructor,
   // but we need a way to instantiate the archetype:
   static distribution_archetype& get_object()
   {
      // will never get caled:
      return *reinterpret_cast<distribution_archetype*>(0);
   }
}; // template <class RealType>class distribution_archetype

// Non-member accessor functions:
// (This list defines the functions that must be implemented by all distributions).

template <class RealType>
RealType pdf(const distribution_archetype<RealType>& dist, const RealType& x);

template <class RealType>
RealType cdf(const distribution_archetype<RealType>& dist, const RealType& x);

template <class RealType>
RealType quantile(const distribution_archetype<RealType>& dist, const RealType& p);

template <class RealType>
RealType cdf(const complemented2_type<distribution_archetype<RealType>, RealType>& c);

template <class RealType>
RealType quantile(const complemented2_type<distribution_archetype<RealType>, RealType>& c);

template <class RealType>
RealType mean(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType standard_deviation(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType variance(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType hazard(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType chf(const distribution_archetype<RealType>& dist);
// http://en.wikipedia.org/wiki/Characteristic_function_%28probability_theory%29

template <class RealType>
RealType coefficient_of_variation(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType mode(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType skewness(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType kurtosis_excess(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType kurtosis(const distribution_archetype<RealType>& dist);

template <class RealType>
RealType median(const distribution_archetype<RealType>& dist);

template <class RealType>
std::pair<RealType, RealType> range(const distribution_archetype<RealType>& dist);

template <class RealType>
std::pair<RealType, RealType> support(const distribution_archetype<RealType>& dist);

//
// Next comes the concept checks for verifying that a class
// fullfils the requirements of a Distribution:
//
template <class Distribution>
struct DistributionConcept
{
   void constraints()
   {
      function_requires<CopyConstructibleConcept<Distribution> >();
      function_requires<AssignableConcept<Distribution> >();

      typedef typename Distribution::value_type value_type;

      const Distribution& dist = DistributionConcept<Distribution>::get_object();

      value_type x = 0;
       // The result values are ignored in all these checks.
       value_type v = cdf(dist, x);
      v = cdf(complement(dist, x));
      v = pdf(dist, x);
      v = quantile(dist, x);
      v = quantile(complement(dist, x));
      v = mean(dist);
      v = mode(dist);
      v = standard_deviation(dist);
      v = variance(dist);
      v = hazard(dist, x);
      v = chf(dist, x);
      v = coefficient_of_variation(dist);
      v = skewness(dist);
      v = kurtosis(dist);
      v = kurtosis_excess(dist);
      v = median(dist);
      std::pair<value_type, value_type> pv;
      pv = range(dist);
      pv = support(dist);

      float f = 1;
      v = cdf(dist, f);
      v = cdf(complement(dist, f));
      v = pdf(dist, f);
      v = quantile(dist, f);
      v = quantile(complement(dist, f));
      v = hazard(dist, f);
      v = chf(dist, f);
      double d = 1;
      v = cdf(dist, d);
      v = cdf(complement(dist, d));
      v = pdf(dist, d);
      v = quantile(dist, d);
      v = quantile(complement(dist, d));
      v = hazard(dist, d);
      v = chf(dist, d);
#ifndef TEST_MPFR
      long double ld = 1;
      v = cdf(dist, ld);
      v = cdf(complement(dist, ld));
      v = pdf(dist, ld);
      v = quantile(dist, ld);
      v = quantile(complement(dist, ld));
      v = hazard(dist, ld);
      v = chf(dist, ld);
#endif
      int i = 1;
      v = cdf(dist, i);
      v = cdf(complement(dist, i));
      v = pdf(dist, i);
      v = quantile(dist, i);
      v = quantile(complement(dist, i));
      v = hazard(dist, i);
      v = chf(dist, i);
      unsigned long li = 1;
      v = cdf(dist, li);
      v = cdf(complement(dist, li));
      v = pdf(dist, li);
      v = quantile(dist, li);
      v = quantile(complement(dist, li));
      v = hazard(dist, li);
      v = chf(dist, li);
   }
private:
   static Distribution& get_object()
   {
      // will never get called:
      static char buf[sizeof(Distribution)];
      return * reinterpret_cast<Distribution*>(buf);
   }
}; // struct DistributionConcept

} // namespace concepts
} // namespace math
} // namespace boost

#endif // BOOST_MATH_DISTRIBUTION_CONCEPT_HPP

