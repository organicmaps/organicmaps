#pragma once

#include "generator/feature_builder.hpp"

#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <string>
#include <utility>
#include <vector>

namespace generator
{
class BoundaryPostcodesEnricher
{
public:
  explicit BoundaryPostcodesEnricher(std::string const & boundaryPostcodesFilename);

  void Enrich(feature::FeatureBuilder & fb) const;

private:
  std::vector<std::pair<std::string, m2::RegionD>> m_boundaryPostcodes;
  m4::Tree<size_t> m_boundariesTree;
};
}  // namespace generator
