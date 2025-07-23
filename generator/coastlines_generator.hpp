#pragma once

#include "generator/feature_merger.hpp"

#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <vector>

namespace feature
{
class FeatureBuilder;
}  // namespace feature

class CoastlineFeaturesGenerator
{
  FeatureMergeProcessor m_merger;

  using TTree = m4::Tree<m2::RegionI>;
  TTree m_tree;

public:
  CoastlineFeaturesGenerator();

  void AddRegionToTree(feature::FeatureBuilder const & fb);

  void Process(feature::FeatureBuilder const & fb);
  /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
  bool Finish();

  std::vector<feature::FeatureBuilder> GetFeatures(size_t maxThreads);
};

namespace coastlines_generator
{
/// @param[in]  poly  Closed polygon where poly.frotn() == poly.back() like in FeatureBuilder.
m2::RegionI CreateRegionI(std::vector<m2::PointD> const & poly);
}  // namespace coastlines_generator
