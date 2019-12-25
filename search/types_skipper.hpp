#pragma once

#include "search/model.hpp"

#include "base/buffer_vector.hpp"

#include <cstdint>

namespace search
{
// Skips some feature's types when feature name is empty.
class TypesSkipper
{
public:
  TypesSkipper();

  void SkipEmptyNameTypes(feature::TypesHolder & types) const;

  // Always skip feature for search index even it has name and other useful types.
  // Useful for mixed features, sponsored objects.
  bool SkipAlways(feature::TypesHolder const & types) const;

private:
  using Cont = buffer_vector<uint32_t, 16>;

  static bool HasType(Cont const & v, uint32_t t);

  // Array index (0, 1) means type level for checking (1, 2).
  Cont m_skipIfEmptyName[2];
  Cont m_skipAlways[2];
};
}  // namespace search
