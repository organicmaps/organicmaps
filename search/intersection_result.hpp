#pragma once

#include "search/model.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace search
{
// This class holds higher-level features for an intersection result,
// i.e. BUILDING and STREET for POI or STREET for BUILDING.
struct IntersectionResult
{
  static uint32_t constexpr kInvalidId = std::numeric_limits<uint32_t>::max();

  void Set(Model::Type type, uint32_t id);

  // Returns the first valid feature among the [POI, BUILDING,
  // STREET].
  uint32_t InnermostResult() const;

  // Returns true when at least one valid feature exists.
  inline bool IsValid() const { return InnermostResult() != kInvalidId; }

  // Clears all fields to an invalid state.
  void Clear();

  uint32_t m_poi = kInvalidId;
  uint32_t m_building = kInvalidId;
  uint32_t m_street = kInvalidId;
};

std::string DebugPrint(IntersectionResult const & result);
}  // namespace search
