#pragma once

#include "search/v2/search_model.hpp"

#include "std/vector.hpp"

#include "base/macros.hpp"

namespace search
{
namespace v2
{
// This structure represents a part of search query interpretation -
// when to a substring of tokens [m_startToken, m_endToken) is matched
// with a set of m_features of the same m_type.
struct FeaturesLayer
{
  FeaturesLayer();
  FeaturesLayer(FeaturesLayer && layer) = default;

  void Clear();

  vector<uint32_t> m_sortedFeatures;

  string m_subQuery;

  size_t m_startToken;
  size_t m_endToken;
  SearchModel::SearchType m_type;

  DISALLOW_COPY(FeaturesLayer);
};

string DebugPrint(FeaturesLayer const & layer);
}  // namespace v2
}  // namespace search
