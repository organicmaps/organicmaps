#pragma once

#include "search/result.hpp"
#include "search/search_quality/sample.hpp"

#include "indexer/index.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

class FeatureType;
struct FeatureID;

namespace search
{
class Matcher
{
public:
  static size_t constexpr kInvalidId = std::numeric_limits<size_t>::max();

  Matcher(Index & index);

  void Match(std::vector<Sample::Result> const & golden, std::vector<Result> const & actual,
             std::vector<size_t> & goldenMatching, std::vector<size_t> & actualMatching);

private:
  WARN_UNUSED_RESULT bool GetFeature(FeatureID const & id, FeatureType & ft);

  bool Matches(Sample::Result const & golden, Result const & actual);

  Index & m_index;
  std::unique_ptr<Index::FeaturesLoaderGuard> m_guard;
};
}  // namespace search
