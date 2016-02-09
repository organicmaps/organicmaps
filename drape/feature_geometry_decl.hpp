#pragma once

#include "indexer/feature_decl.hpp"

namespace dp
{

struct FeatureGeometryId
{
  FeatureID m_featureId;
  uint32_t m_shapeInd = 0;

  FeatureGeometryId() {}
  FeatureGeometryId(FeatureID feature, uint32_t shapeInd)
    : m_featureId(feature)
    , m_shapeInd(shapeInd)
  {}

  bool IsValid() const { return m_featureId.IsValid(); }
  inline bool operator<(FeatureGeometryId const & r) const
  {
    if (m_featureId == r.m_featureId)
      return m_shapeInd < r.m_shapeInd;
    return m_featureId < r.m_featureId;
  }
};

} // namespace dp
