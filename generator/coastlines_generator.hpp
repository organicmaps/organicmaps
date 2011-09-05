#pragma once

#include "feature_merger.hpp"

#include "../indexer/cell_id.hpp"

#include "../geometry/tree4d.hpp"
#include "../geometry/region2d.hpp"


class FeatureBuilder1;

class CoastlineFeaturesGenerator
{
  typedef RectId CellIdT;

  FeatureMergeProcessor m_merger;

  m4::Tree<m2::RegionI> m_tree;

  uint32_t m_coastType;
  int m_Level;

public:
  CoastlineFeaturesGenerator(uint32_t coastType, int level = 6);

  void AddRegionToTree(FeatureBuilder1 const & fb);

  void operator() (FeatureBuilder1 const & fb);
  void Finish();

  inline size_t GetFeaturesCount() const { return 1 << 2 * m_Level; }
  bool GetFeature(size_t i, FeatureBuilder1 & fb);
};
