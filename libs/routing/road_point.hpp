#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

#include "std/boost_container_hash.hpp"

namespace routing
{
// RoadPoint is a unique identifier for any road point in mwm file.
//
// Contains feature id and point id.
// Point id is the ordinal number of the point in the road.
class RoadPoint final
{
public:
  RoadPoint() : m_featureId(0), m_pointId(0) {}

  RoadPoint(uint32_t featureId, uint32_t pointId) : m_featureId(featureId), m_pointId(pointId) {}

  uint32_t GetFeatureId() const { return m_featureId; }

  uint32_t GetPointId() const { return m_pointId; }

  bool operator<(RoadPoint const & rp) const
  {
    if (m_featureId != rp.m_featureId)
      return m_featureId < rp.m_featureId;
    return m_pointId < rp.m_pointId;
  }

  bool operator==(RoadPoint const & rp) const { return m_featureId == rp.m_featureId && m_pointId == rp.m_pointId; }

  bool operator!=(RoadPoint const & rp) const { return !(*this == rp); }

  struct Hash
  {
    size_t operator()(RoadPoint const & roadPoint) const
    {
      size_t seed = 0;
      boost::hash_combine(seed, roadPoint.m_featureId);
      boost::hash_combine(seed, roadPoint.m_pointId);
      return seed;
    }
  };

  void SetPointId(uint32_t pointId) { m_pointId = pointId; }

private:
  uint32_t m_featureId;
  uint32_t m_pointId;
};

inline std::string DebugPrint(RoadPoint const & rp)
{
  std::ostringstream out;
  out << "RoadPoint{" << rp.GetFeatureId() << ", " << rp.GetPointId() << "}";
  return out.str();
}
}  // namespace routing
