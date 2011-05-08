#pragma once

#include "../indexer/feature.hpp"

class FeatureBuilder1Merger : public FeatureBuilder1
{
public:
  FeatureBuilder1Merger(FeatureBuilder1 const & fb);

  /// adds fb's geometry at the end of own geometry,
  /// but only if they have common point
  void AppendFeature(FeatureBuilder1Merger const & fb);

  bool SetAreaSafe()
  {
    if (m_Geometry.size() < 3)
      return false;

    m_Params.SetGeomType(feature::GEOM_AREA);
    return true;
  }

  uint32_t KeyType() const
  {
    return m_Params.KeyType();
  }

  bool ReachedMaxPointsCount() const;

  m2::PointD FirstPoint() const { return m_Geometry.front(); }
  m2::PointD LastPoint() const { return m_Geometry.back(); }
};
