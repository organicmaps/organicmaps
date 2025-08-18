#include "openlr/road_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "base/assert.hpp"

namespace openlr
{
// RoadInfoGetter::RoadInfo ------------------------------------------------------------------------
RoadInfoGetter::RoadInfo::RoadInfo(FeatureType & ft)
  : m_hwClass(ftypes::GetHighwayClass(feature::TypesHolder(ft)))
  , m_link(ftypes::IsLinkChecker::Instance()(ft))
  , m_oneWay(ftypes::IsOneWayChecker::Instance()(ft))
  , m_isRoundabout(ftypes::IsRoundAboutChecker::Instance()(ft))
{}

// RoadInfoGetter ----------------------------------------------------------------------------------
RoadInfoGetter::RoadInfoGetter(DataSource const & dataSource) : m_dataSource(dataSource) {}

RoadInfoGetter::RoadInfo RoadInfoGetter::Get(FeatureID const & fid)
{
  auto it = m_cache.find(fid);
  if (it != end(m_cache))
    return it->second;

  FeaturesLoaderGuard g(m_dataSource, fid.m_mwmId);
  auto ft = g.GetOriginalFeatureByIndex(fid.m_index);
  CHECK(ft, ());

  RoadInfo info(*ft);
  it = m_cache.emplace(fid, info).first;

  return it->second;
}
}  // namespace openlr
