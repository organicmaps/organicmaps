//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_DISTRIBUTIONS_DETAIL_INV_DISCRETE_QUANTILE
#define BOOST_MATH_DISTRIBUTIONS_DETAIL_INV_DISCRETE_QUANTILE

#include <algorithm>

namespace boost{ namespace math{ namespace detail{

//
// Functor for root finding algorithm:
//
template <class Dist>
struct distribution_quantile_finder
{
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;

   distribution_quantile_finder(const Dist d, value_type p, value_type q)
      : dist(d), target(p < q ? p : q), comp(p < q ? false : true) {}

   value_type operator()(value_type const& x)
   {
      return comp ? target - cdf(complement(dist, x)) : cdf(dist, x) - target;
   }

private:
   Dist dist;
   value_type target;
   bool comp;
};
//
// The purpose of adjust_bounds, is to toggle the last bit of the
// range so that both ends round to the same integer, if possible.
// If they do both round the same then we terminate the search
// for the root *very* quickly when finding an integer result.
// At the point that this function is called we know that "a" is
// below the root and "b" above it, so this change can not result
// in the root no longer being bracketed.
//
template <class Real, class Tol>
void adjust_bounds(Real& /* a */, Real& /* b */, Tol const& /* tol */){}

template <class Real>
void adjust_bounds(Real& /* a */, Real& b, tools::equal_floor const& /* tol */)
{
   BOOST_MATH_STD_USING
   b -= tools::epsilon<Real>() * b;
}

template <class Real>
void adjust_bounds(Real& a, Real& /* b */, tools::equal_ceil const& /* tol */)
{
   BOOST_MATH_STD_USING
   a += tools::epsilon<Real>() * a;
}

template <class Real>
void adjust_bounds(Real& a, Real& b, tools::equal_nearest_integer const& /* tol */)
{
   BOOST_MATH_STD_USING
   a += tools::epsilon<Real>() * a;
   b -= tools::epsilon<Real>() * b;
}
//
// This is where all the work is done:
//
template <class Dist, class Tolerance>
typename Dist::value_type 
   do_inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      typename Dist::value_type guess,
      const typename Dist::value_type& multiplier,
      typename Dist::value_type adder,
      const Tolerance& tol,
      boost::uintmax_t& max_iter)
{
   typedef typename Dist::value_type value_type;
   typedef typename Dist::policy_type policy_type;

   static const char* function = "boost::math::do_inverse_discrete_quantile<%1%>";

   BOOST_MATH_STD_USING

   distribution_quantile_finder<Dist> f(dist, p, q);
   //
   // Max bounds of the distribution:
   //
   value_type min_bound, max_bound;
   std::tr1::tie(min_bound, max_bound) = support(dist);

   if(guess > max_bound)
      guess = max_bound;
   if(guess < min_bound)
      guess = min_bound;

   value_type fa = f(guess);
   boost::uintmax_t count = max_iter - 1;
   value_type fb(fa), a(guess), b =0; // Compiler warning C4701: potentially uninitialized local variable 'b' used

   if(fa == 0)
      return guess;

   //
   // For small expected results, just use a linear search:
   //
   if(guess < 10)
   {
      b = a;
      while((a < 10) && (fa * fb >= 0))
      {
         if(fb <= 0)
         {
            a = b;
            b = a + 1;
            if(b > max_bound)
               b = max_bound;
            fb = f(b);
            --count;
            if(fb == 0)
               return b;
         }
         else
         {
            b = a;
            a = (std::max)(value_type(b - 1), value_type(0));
            if(a < min_bound)
               a = min_bound;
            fa = f(a);
            --count;
            if(fa == 0)
               return a;
         }
      }
   }
   //
   // Try and bracket using a couple of additions first, 
   // we're assuming that "guess" is likely to be accurate
   // to the nearest int or so:
   //
   else if(adder != 0)
   {
      //
      // If we're looking for a large result, then bump "adder" up
      // by a bit to increase our chances of bracketing the root:
      //
      //adder = (std::max)(adder, 0.001f * guess);
      if(fa < 0)
      {
         b = a + adder;
         if(b > max_bound)
            b = max_bound;
      }
      else
      {
         b = (std::max)(value_type(a - adder), value_type(0));
         if(b < min_bound)
            b = min_bound;
      }
      fb = f(b);
      --count;
      if(fb == 0)
         return b;
      if(count && (fa * fb >= 0))
      {
         //
         // We didn't bracket the root, try 
         // once more:
         //
         a = b;
         fa = fb;
         if(fa < 0)
         {
            b = a + adder;
            if(b > max_bound)
               b = max_bound;
         }
         else
         {
            b = (std::max)(value_type(a - adder), value_type(0));
            if(b < min_bound)
               b = min_bound;
         }
         fb = f(b);
         --count;
      }
      if(a > b)
      {
         using std::swap;
         swap(a, b);
         swap(fa, fb);
      }
   }
   //
   // If the root hasn't been bracketed yet, try again
   // using the multiplier this time:
   //
   if((boost::math::sign)(fb) == (boost::math::sign)(fa))
   {
      if(fa < 0)
      {
         //
         // Zero is to the right of x2, so walk upwards
         // until we find it:
         //
         while((boost::math::sign)(fb) == (boost::math::sign)(fa))
         {
            if(count == 0)
               policies::raise_evaluation_error(function, "Unable to bracket root, last nearest value was %1%", b, policy_type());
            a = b;
            fa = fb;
            b *= multiplier;
            if(b > max_bound)
               b = max_bound;
            fb = f(b);
            --count;
            BOOST_MATH_INSTRUMENT_CODE("a = " << a << " b = " << b << " fa = " << fa << " fb = " << fb << " count = " << count);
         }
      }
      else
      {
         //
         // Zero is to the left of a, so walk downwards
         // until we find it:
         //
         while((boost::math::sign)(fb) == (boost::math::sign)(fa))
         {
            if(fabs(a) < tools::min_value<value_type>())
            {
               // Escape route just in case the answer is zero!
               max_iter -= count;
               max_iter += 1;
               return 0;
            }
            if(count == 0)
               policies::raise_evaluation_error(function, "Unable to bracket root, last nearest value was %1%", a, policy_type());
            b = a;
            fb = fa;
            a /= multiplier;
            if(a < min_bound)
               a = min_bound;
            fa = f(a);
            --count;
            BOOST_MATH_INSTRUMENT_CODE("a = " << a << " b = " << b << " fa = " << fa << " fb = " << fb << " count = " << count);
         }
      }
   }
   max_iter -= count;
   if(fa == 0)
      return a;
   if(fb == 0)
      return b;
   //
   // Adjust bounds so that if we're looking for an integer
   // result, then both ends round the same way:
   //
   adjust_bounds(a, b, tol);
   //
   // We don't want zero or denorm lower bounds:
   //
   if(a < tools::min_value<value_type>())
      a = tools::min_value<value_type>();
   //
   // Go ahead and find the root:
   //
   std::pair<value_type, value_type> r = toms748_solve(f, a, b, fa, fb, tol, count, policy_type());
   max_iter += count;
   BOOST_MATH_INSTRUMENT_CODE("max_iter = " << max_iter << " count = " << count);
   return (r.first + r.second) / 2;
}
//
// Now finally are the public API functions.
// There is one overload for each policy,
// each one is responsible for selecting the correct
// termination condition, and rounding the result
// to an int where required.
//
template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::real>&,
      boost::uintmax_t& max_iter)
{
   if(p <= pdf(dist, 0))
      return 0;
   return do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      guess, 
      multiplier, 
      adder, 
      tools::eps_tolerance<typename Dist::value_type>(policies::digits<typename Dist::value_type, typename Dist::policy_type>()),
      max_iter);
}

template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::integer_round_outwards>&,
      boost::uintmax_t& max_iter)
{
   typedef typename Dist::value_type value_type;
   BOOST_MATH_STD_USING
   if(p <= pdf(dist, 0))
      return 0;
   //
   // What happens next depends on whether we're looking for an 
   // upper or lower quantile:
   //
   if(p < 0.5f)
      return floor(do_inverse_discrete_quantile(
         dist, 
         p, 
         q,
         (guess < 1 ? value_type(1) : (value_type)floor(guess)), 
         multiplier, 
         adder, 
         tools::equal_floor(),
         max_iter));
   // else:
   return ceil(do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      (value_type)ceil(guess), 
      multiplier, 
      adder, 
      tools::equal_ceil(),
      max_iter));
}

template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::integer_round_inwards>&,
      boost::uintmax_t& max_iter)
{
   typedef typename Dist::value_type value_type;
   BOOST_MATH_STD_USING
   if(p <= pdf(dist, 0))
      return 0;
   //
   // What happens next depends on whether we're looking for an 
   // upper or lower quantile:
   //
   if(p < 0.5f)
      return ceil(do_inverse_discrete_quantile(
         dist, 
         p, 
         q,
         ceil(guess), 
         multiplier, 
         adder, 
         tools::equal_ceil(),
         max_iter));
   // else:
   return floor(do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      (guess < 1 ? value_type(1) : floor(guess)), 
      multiplier, 
      adder, 
      tools::equal_floor(),
      max_iter));
}

template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::integer_round_down>&,
      boost::uintmax_t& max_iter)
{
   typedef typename Dist::value_type value_type;
   BOOST_MATH_STD_USING
   if(p <= pdf(dist, 0))
      return 0;
   return floor(do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      (guess < 1 ? value_type(1) : floor(guess)), 
      multiplier, 
      adder, 
      tools::equal_floor(),
      max_iter));
}

template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::integer_round_up>&,
      boost::uintmax_t& max_iter)
{
   BOOST_MATH_STD_USING
   if(p <= pdf(dist, 0))
      return 0;
   return ceil(do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      ceil(guess), 
      multiplier, 
      adder, 
      tools::equal_ceil(),
      max_iter));
}

template <class Dist>
inline typename Dist::value_type 
   inverse_discrete_quantile(
      const Dist& dist,
      const typename Dist::value_type& p,
      const typename Dist::value_type& q,
      const typename Dist::value_type& guess,
      const typename Dist::value_type& multiplier,
      const typename Dist::value_type& adder,
      const policies::discrete_quantile<policies::integer_round_nearest>&,
      boost::uintmax_t& max_iter)
{
   typedef typename Dist::value_type value_type;
   BOOST_MATH_STD_USING
   if(p <= pdf(dist, 0))
      return 0;
   //
   // Note that we adjust the guess to the nearest half-integer:
   // this increase the chances that we will bracket the root
   // with two results that both round to the same integer quickly.
   //
   return floor(do_inverse_discrete_quantile(
      dist, 
      p, 
      q,
      (guess < 0.5f ? value_type(1.5f) : floor(guess + 0.5f) + 0.5f), 
      multiplier, 
      adder, 
      tools::equal_nearest_integer(),
      max_iter) + 0.5f);
}

}}} // namespaces

#endif // BOOST_MATH_DISTRIBUTIONS_DETAIL_INV_DISCRETE_QUANTILE


