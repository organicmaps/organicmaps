#pragma once

#include "search/search_model.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"

namespace search
{
// This class holds higher-level features for an intersection result,
// i.e. BUILDING and STREET for POI or STREET for BUILDING.
struct IntersectionResult
{
  static uint32_t const kInvalidId;

  IntersectionResult();

  void Set(SearchModel::SearchType type, uint32_t id);

  // Returns the first valid feature among the [POI, BUILDING,
  // STREET].
  uint32_t InnermostResult() const;

  // Returns true when at least one valid feature exists.
  inline bool IsValid() const { return InnermostResult() != kInvalidId; }

  // Clears all fields to an invalid state.
  void Clear();

  uint32_t m_poi;
  uint32_t m_building;
  uint32_t m_street;
};

string DebugPrint(IntersectionResult const & result);

}  // namespace search
