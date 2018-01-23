#pragma once

#include "search/model.hpp"

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
  using TCont = buffer_vector<uint32_t, 16>;

  static bool HasType(TCont const & v, uint32_t t);

  // Array index (0, 1) means type level for checking (1, 2).
  // m_skipAlways is used in the case 1 described above.
  TCont m_skipAlways[2];

  // m_skipIfEmptyName and m_dontSkipIfEmptyName are used in the case 2 described above.
  TCont m_skipIfEmptyName[2];
  TwoLevelPOIChecker m_dontSkipIfEmptyName;

  uint32_t m_country, m_state;
};
}  // namespace search
