#pragma once

#include "generator/feature_builder.hpp"
#include "generator/regions/region_base.hpp"
#include "generator/regions/region_info_collector.hpp"

namespace generator
{
namespace regions
{
struct City : public RegionWithName, public RegionWithData
{
  explicit City(FeatureBuilder1 const & fb, RegionDataProxy const & rd)
    : RegionWithName(fb.GetParams().name),
      RegionWithData(rd)
  {
    auto const p = fb.GetKeyPoint();
    m_center = {p.x, p.y};
  }

  BoostPoint GetCenter() const {  return m_center; }

private:
  BoostPoint m_center;
};
}  // namespace regions
}  // namespace generator
