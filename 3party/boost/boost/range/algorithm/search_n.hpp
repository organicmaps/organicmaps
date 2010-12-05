//  Copyright Neil Groves 2009. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ALGORITHM_SEARCH_N_HPP_INCLUDED
#define BOOST_RANGE_ALGORITHM_SEARCH_N_HPP_INCLUDED

#include <boost/concept_check.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/range/value_type.hpp>
#include <algorithm>

namespace boost
{
    namespace range
    {

/// \brief template function search
///
/// range-based version of the search std algorithm
///
/// \pre ForwardRange is a model of the ForwardRangeConcept
/// \pre Integer is an integral type
/// \pre Value is a model of the EqualityComparableConcept
/// \pre ForwardRange's value type is a model of the EqualityComparableConcept
/// \pre Object's of ForwardRange's value type can be compared for equality with Objects of type Value
template< class ForwardRange, class Integer, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
search_n(ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return std::search_n(boost::begin(rng),boost::end(rng), count, value);
}

/// \overload
template< class ForwardRange, class Integer, class Value >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
search_n(const ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    return std::search_n(boost::begin(rng), boost::end(rng), count, value);
}

/// \overload
template< class ForwardRange, class Integer, class Value,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<ForwardRange>::type
search_n(ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate binary_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<ForwardRange>::type, const Value&>));
    return std::search_n(boost::begin(rng), boost::end(rng),
        count, value, binary_pred);
}

/// \overload
template< class ForwardRange, class Integer, class Value,
          class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_iterator<const ForwardRange>::type
search_n(const ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate binary_pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<const ForwardRange>::type, const Value&>));
    return std::search_n(boost::begin(rng), boost::end(rng),
        count, value, binary_pred);
}

// range_return overloads

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
search_n(ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    return range_return<ForwardRange,re>::
        pack(std::search_n(boost::begin(rng),boost::end(rng),
                           count, value),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
search_n(const ForwardRange& rng, Integer count, const Value& value)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    return range_return<const ForwardRange,re>::
        pack(std::search_n(boost::begin(rng), boost::end(rng),
                           count, value),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<ForwardRange,re>::type
search_n(ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<ForwardRange>::type,
        const Value&>));
    return range_return<ForwardRange,re>::
        pack(std::search_n(boost::begin(rng), boost::end(rng),
                           count, value, pred),
             rng);
}

/// \overload
template< range_return_value re, class ForwardRange, class Integer,
          class Value, class BinaryPredicate >
inline BOOST_DEDUCED_TYPENAME range_return<const ForwardRange,re>::type
search_n(const ForwardRange& rng, Integer count, const Value& value,
         BinaryPredicate pred)
{
    BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRange>));
    BOOST_RANGE_CONCEPT_ASSERT((BinaryPredicateConcept<BinaryPredicate,
        BOOST_DEDUCED_TYPENAME range_value<const ForwardRange>::type,
        const Value&>));
    return range_return<const ForwardRange,re>::
        pack(std::search_n(boost::begin(rng), boost::end(rng),
                           count, value, pred),
             rng);
}

    } // namespace range
    using range::search_n;
} // namespace boost

#endif // include guard
