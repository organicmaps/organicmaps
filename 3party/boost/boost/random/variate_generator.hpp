/* boost random/variate_generator.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id: variate_generator.hpp 60755 2010-03-22 00:45:06Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_RANDOM_GENERATOR_HPP
#define BOOST_RANDOM_RANDOM_GENERATOR_HPP

#include <boost/config.hpp>

// implementation details
#include <boost/detail/workaround.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/detail/pass_through_engine.hpp>
#include <boost/random/detail/uniform_int_float.hpp>
#include <boost/random/detail/ptr_helper.hpp>

// Borland C++ 5.6.0 has problems using its numeric_limits traits as
// template parameters
#if BOOST_WORKAROUND(__BORLANDC__, <= 0x564)
#include <boost/type_traits/is_integral.hpp>
#endif

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {

/// \cond hide_private_members

namespace random {
namespace detail {

template<bool have_int, bool want_int>
struct engine_helper;

// for consistency, always have two levels of decorations
template<>
struct engine_helper<true, true>
{
  template<class Engine, class DistInputType>
  struct impl
  {
    typedef pass_through_engine<Engine> type;
  };
};

template<>
struct engine_helper<false, false>
{
  template<class Engine, class DistInputType>
  struct impl
  {
    typedef uniform_01<Engine, DistInputType> type;
  };
};

template<>
struct engine_helper<true, false>
{
  template<class Engine, class DistInputType>
  struct impl
  {
    typedef uniform_01<Engine, DistInputType> type;
  };
};

template<>
struct engine_helper<false, true>
{
  template<class Engine, class DistInputType>
  struct impl
  {
    typedef uniform_int_float<Engine, unsigned long> type;
  };
};

} // namespace detail
} // namespace random

///\endcond

/**
 * A random variate generator is used to join a random number
 * generator together with a random number distribution.
 * Boost.Random provides a vast choice of \generators as well
 * as \distributions.
 *
 * Instantations of class template @c variate_generator model
 * a \number_generator.
 *
 * The argument for the template parameter Engine shall be of
 * the form U, U&, or U*, where U models a
 * \uniform_random_number_generator.  Then, the member
 * engine_value_type names U (not the pointer or reference to U).
 *
 * Specializations of @c variate_generator satisfy the
 * requirements of CopyConstructible. They also satisfy the
 * requirements of Assignable unless the template parameter
 * Engine is of the form U&.
 *
 * The complexity of all functions specified in this section
 * is constant. No function described in this section except
 * the constructor throws an exception.
 */
template<class Engine, class Distribution>
class variate_generator
{
private:
  typedef random::detail::pass_through_engine<Engine> decorated_engine;

public:
  typedef typename decorated_engine::base_type engine_value_type;
  typedef Engine engine_type;
  typedef Distribution distribution_type;
  typedef typename Distribution::result_type result_type;

  /**
   * Constructs a @c variate_generator object with the associated
   * \uniform_random_number_generator eng and the associated
   * \random_distribution d.
   *
   * Throws: If and what the copy constructor of Engine or
   * Distribution throws.
   */
  variate_generator(Engine e, Distribution d)
    : _eng(decorated_engine(e)), _dist(d) { }

  /**
   * Returns: distribution()(e)
   *
   * Notes: The sequence of numbers produced by the
   * \uniform_random_number_generator e, s<sub>e</sub>, is
   * obtained from the sequence of numbers produced by the
   * associated \uniform_random_number_generator eng, s<sub>eng</sub>,
   * as follows: Consider the values of @c numeric_limits<T>::is_integer
   * for @c T both @c Distribution::input_type and
   * @c engine_value_type::result_type. If the values for both types are
   * true, then se is identical to s<sub>eng</sub>. Otherwise, if the
   * values for both types are false, then the numbers in s<sub>eng</sub>
   * are divided by engine().max()-engine().min() to obtain the numbers
   * in s<sub>e</sub>. Otherwise, if the value for
   * @c engine_value_type::result_type is true and the value for
   * @c Distribution::input_type is false, then the numbers in s<sub>eng</sub>
   * are divided by engine().max()-engine().min()+1 to obtain the numbers in
   * s<sub>e</sub>. Otherwise, the mapping from s<sub>eng</sub> to
   * s<sub>e</sub> is implementation-defined. In all cases, an
   * implicit conversion from @c engine_value_type::result_type to
   * @c Distribution::input_type is performed. If such a conversion does
   * not exist, the program is ill-formed.
   */
  result_type operator()() { return _dist(_eng); }
  /**
   * Returns: distribution()(e, value).
   * For the semantics of e, see the description of operator()().
   */
  template<class T>
  result_type operator()(T value) { return _dist(_eng, value); }

  /**
   * Returns: A reference to the associated uniform random number generator.
   */
  engine_value_type& engine() { return _eng.base().base(); }
  /**
   * Returns: A reference to the associated uniform random number generator.
   */
  const engine_value_type& engine() const { return _eng.base().base(); }

  /**
   * Returns: A reference to the associated random distribution.
   */
  distribution_type& distribution() { return _dist; }
  /**
   * Returns: A reference to the associated random distribution.
   */
  const distribution_type& distribution() const { return _dist; }

  /**
   * Precondition: distribution().min() is well-formed
   *
   * Returns: distribution().min()
   */
  result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (distribution().min)(); }
  /**
   * Precondition: distribution().max() is well-formed
   *
   * Returns: distribution().max()
   */
  result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (distribution().max)(); }

private:
#if BOOST_WORKAROUND(__BORLANDC__, <= 0x564)
  typedef typename random::detail::engine_helper<
    ::boost::is_integral<typename decorated_engine::result_type>::value,
    ::boost::is_integral<typename Distribution::input_type>::value
    >::BOOST_NESTED_TEMPLATE impl<decorated_engine, typename Distribution::input_type>::type internal_engine_type;
#else
  enum {
    have_int = std::numeric_limits<typename decorated_engine::result_type>::is_integer,
    want_int = std::numeric_limits<typename Distribution::input_type>::is_integer
  };
  typedef typename random::detail::engine_helper<have_int, want_int>::BOOST_NESTED_TEMPLATE impl<decorated_engine, typename Distribution::input_type>::type internal_engine_type;
#endif

  internal_engine_type _eng;
  distribution_type _dist;
};

} // namespace boost

#include <boost/random/detail/disable_warnings.hpp>

#endif // BOOST_RANDOM_RANDOM_GENERATOR_HPP
