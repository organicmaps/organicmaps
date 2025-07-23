#pragma once

#include "search/search_quality/sample.hpp"

#include "search/result.hpp"

#include "base/string_utils.hpp"

#include <cstddef>
#include <limits>
#include <vector>

class FeatureType;

namespace search
{
class FeatureLoader;

class Matcher
{
public:
  inline static size_t constexpr kInvalidId = std::numeric_limits<size_t>::max();

  explicit Matcher(FeatureLoader & loader);

  // Matches the results loaded from |goldenSample| with |actual| results
  // found by the search engine using the params from the Sample.
  // goldenMatching[i] is the index of the result in |actual| that matches
  // the sample result number i.
  // actualMatching[j] is the index of the sample in |golden| that matches
  // the golden result number j.
  void Match(Sample const & goldenSample, std::vector<Result> const & actual, std::vector<size_t> & goldenMatching,
             std::vector<size_t> & actualMatching);

  bool Matches(strings::UniString const & query, Sample::Result const & golden, Result const & actual);
  bool Matches(strings::UniString const & query, Sample::Result const & golden, FeatureType & ft);

private:
  FeatureLoader & m_loader;
};
}  // namespace search
