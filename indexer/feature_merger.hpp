#pragma once

#include "feature.hpp"

class FeatureBuilder1Merger : public FeatureBuilder1
{
public:
  FeatureBuilder1Merger(FeatureBuilder1 const & fb);
  bool MergeWith(FeatureBuilder1 const & fb);

  vector<uint32_t> const & Type() const { return m_Types; }
  bool ReachedMaxPointsCount() const;
};
