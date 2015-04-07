#pragma once

#include "generator/feature_merger.hpp"

#include "indexer/cell_id.hpp"

#include "geometry/tree4d.hpp"
#include "geometry/region2d.hpp"


class FeatureBuilder1;

class CoastlineFeaturesGenerator
{
  typedef RectId CellIdT;

  FeatureMergeProcessor m_merger;

  typedef m4::Tree<m2::RegionI> TreeT;
  TreeT m_tree;

  uint32_t m_coastType;
  int m_lowLevel, m_highLevel, m_maxPoints;

  bool GetFeature(CellIdT const & cell, FeatureBuilder1 & fb);

public:
  CoastlineFeaturesGenerator(uint32_t coastType,
                             int lowLevel, int highLevel, int maxPoints);

  void AddRegionToTree(FeatureBuilder1 const & fb);

  void operator() (FeatureBuilder1 const & fb);
  /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
  bool Finish();

  inline size_t GetCellsCount() const { return 1 << 2 * m_lowLevel; }
  void GetFeatures(size_t i, vector<FeatureBuilder1> & vecFb);
};
