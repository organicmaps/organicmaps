#pragma once

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include <cstdint>
#include <limits>
#include <vector>

class FeatureType;

namespace search
{
class FeatureLoader;

class Matcher
{
public:
  static size_t constexpr kInvalidId = std::numeric_limits<size_t>::max();

  explicit Matcher(FeatureLoader & loader);

  void Match(std::vector<Sample::Result> const & golden, std::vector<Result> const & actual,
             std::vector<size_t> & goldenMatching, std::vector<size_t> & actualMatching);

  bool Matches(Sample::Result const & golden, FeatureType & ft);
  bool Matches(Sample::Result const & golden, Result const & actual);

private:
  FeatureLoader & m_loader;
};
}  // namespace search
