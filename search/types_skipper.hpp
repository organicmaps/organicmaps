#pragma once

#include "search/model.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/buffer_vector.hpp"

namespace search
{
// There are 2 different ways of search index skipping:
// 1. Skip some feature's types.
// 2. Skip some feature's types when feature name is empty.
class TypesSkipper
{
public:
  TypesSkipper();

  void SkipTypes(feature::TypesHolder & types) const;

  void SkipEmptyNameTypes(feature::TypesHolder & types) const;

  bool IsCountryOrState(feature::TypesHolder const & types) const;

private:
  class DontSkipIfEmptyName
  {
  public:
    bool IsMatched(uint32_t type) const
    {
      // This is needed for Cian support.
      auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
      return m_poiChecker.IsMatched(type) || buildingChecker.IsMatched(type);
    }

  private:
    TwoLevelPOIChecker m_poiChecker;
  };

  using TCont = buffer_vector<uint32_t, 16>;

  static bool HasType(TCont const & v, uint32_t t);

  // Array index (0, 1) means type level for checking (1, 2).
  // m_skipAlways is used in the case 1 described above.
  TCont m_skipAlways[2];

  // m_skipIfEmptyName and m_dontSkipIfEmptyName are used in the case 2 described above.
  TCont m_skipIfEmptyName[2];
  DontSkipIfEmptyName m_dontSkipIfEmptyName;

  uint32_t m_country, m_state;
};
}  // namespace search
