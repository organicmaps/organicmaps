#pragma once

#include "base/buffer_vector.hpp"

namespace feature
{
class TypesHolder;
}

namespace search
{
/// There are 3 different ways of search index skipping:
/// - skip features in any case (m_skipFeatures)
/// - skip features with empty names (m_enFeature)
/// - skip specified types for features with empty names (m_enTypes)
class TypesSkipper
{
public:
  TypesSkipper();

  void SkipTypes(feature::TypesHolder & types) const;

  void SkipEmptyNameTypes(feature::TypesHolder & types) const;

  bool IsCountryOrState(feature::TypesHolder const & types) const;

private:
  using TCont = buffer_vector<uint32_t, 16>;

  static bool HasType(TCont const & v, uint32_t t);

  // Array index (0, 1) means type level for checking (1, 2).
  TCont m_skipEn[2];
  TCont m_skipF[2];
  TCont m_dontSkipEn;
  uint32_t m_country, m_state;
};
}  // namespace search
