#pragma once

#include "feature.hpp"

class FeatureBuilder1Merger : public FeatureBuilder1
{
public:
  FeatureBuilder1Merger(FeatureBuilder1 const & fb);
  bool MergeWith(FeatureBuilder1 const & fb);
};
