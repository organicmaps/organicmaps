#pragma once

#include "search/cbv.hpp"
#include "search/model.hpp"
#include "search/token_range.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace search
{
// This structure represents a part of search query interpretation -
// when to a substring of tokens [m_startToken, m_endToken) is matched
// with a set of m_features of the same m_type.
struct FeaturesLayer
{
  FeaturesLayer();

  void Clear();

  // Non-owning ptr to a sorted vector of features.
  std::vector<uint32_t> const * m_sortedFeatures = nullptr;
  // Fetch vector of Features, described by this layer (used for CITY, SUBURB).
  std::function<CBV()> m_getFeatures;

  strings::UniString m_subQuery;
  TokenRange m_tokenRange;
  Model::Type m_type;

  // Meaningful only when m_type equals to BUILDING.
  // When true, m_sortedFeatures contains only features retrieved from
  // search index by m_subQuery, and it's necessary for Geocoder to
  // perform additional work to retrieve features matching by house number.
  bool m_hasDelayedFeatures;

  bool m_lastTokenIsPrefix;
};

std::string DebugPrint(FeaturesLayer const & layer);
}  // namespace search
