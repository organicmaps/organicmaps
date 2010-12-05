//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_FOR_EACH_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_FOR_EACH_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function for_each
///
/// range-based version of the for_each std algorithm
///
/// \pre SinglePassRange is a model of the SinglePassRangeConcept
/// \pre UnaryFunction is a model of the UnaryFunctionConcept
template< class SinglePassRange, class UnaryFunction >
inline UnaryFunction for_each(SinglePassRange & rng, UnaryFunction fun)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<SinglePassRange> ));
    return std::for_each(boost::begin(rng),boost::end(rng),fun);
}

/// \overload
template< class SinglePassRange, class UnaryFunction >
inline UnaryFunction for_each(const SinglePassRange& rng, UnaryFunction fun)
{
    BOOST_RANGE_CONCEPT_ASSERT(( SinglePassRangeConcept<const SinglePassRange> ));
    return std::for_each(boost::begin(rng), boost::end(rng), fun);
}

    } // namespace range
    using range::for_each;
} // namespace boost

#endif // include guard
