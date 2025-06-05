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

  // Returns the first valid feature among the [SUBPOI, COMPLEX_POI, BUILDING, STREET].
  uint32_t InnermostResult() const;

  // Returns true when at least one valid feature exists.
  inline bool IsValid() const { return InnermostResult() != kInvalidId; }

  // Building == Streets means that we have actual street result, but got here
  // via _fake_ TYPE_BUILDING layer (see MatchPOIsAndBuildings).
  inline bool IsFakeBuildingButStreet() const { return m_building != kInvalidId && m_building == m_street; }

  inline bool IsPoiAndComplexPoi() const { return m_complexPoi != kInvalidId && m_subpoi != kInvalidId; }

  // Clears all fields to an invalid state.
  void Clear();

  uint32_t m_subpoi = kInvalidId;
  uint32_t m_complexPoi = kInvalidId;
  uint32_t m_building = kInvalidId;
  uint32_t m_street = kInvalidId;
  uint32_t m_suburb = kInvalidId;
};

std::string DebugPrint(IntersectionResult const & result);
}  // namespace search
