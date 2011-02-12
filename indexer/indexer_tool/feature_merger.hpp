#pragma once

#include "../feature.hpp"

class FeatureBuilder1Merger : public FeatureBuilder1
{
public:
  FeatureBuilder1Merger(FeatureBuilder1 const & fb);

  /// adds fb's geometry at the end of own geometry,
  /// but only if they have common point
  void AppendFeature(FeatureBuilder1Merger const & fb);

  vector<uint32_t> const & Type() const { return m_Types; }
  bool ReachedMaxPointsCount() const;

  m2::PointD FirstPoint() const { return m_Geometry.front(); }
  m2::PointD LastPoint() const { return m_Geometry.back(); }
};
