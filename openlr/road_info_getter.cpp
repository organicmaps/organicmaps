#include "openlr/road_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_source.hpp"

#include "base/assert.hpp"

namespace openlr
{
RoadInfoGetter::RoadInfoGetter(DataSource const & dataSource)
  : m_dataSource(dataSource)
{
}

RoadInfoGetter::RoadInfo RoadInfoGetter::Get(FeatureID const & fid)
{
  auto it = m_cache.find(fid);
  if (it != end(m_cache))
    return it->second;

  FeaturesLoaderGuard g(m_dataSource, fid.m_mwmId);
  FeatureType ft;
  CHECK(g.GetOriginalFeatureByIndex(fid.m_index, ft), ());

  RoadInfo info;
  info.m_hwClass = ftypes::GetHighwayClass(feature::TypesHolder(ft));
  info.m_link = ftypes::IsLinkChecker::Instance()(ft);
  info.m_oneWay = ftypes::IsOneWayChecker::Instance()(ft);

  it = m_cache.emplace(fid, info).first;

  return it->second;
}
}  // namespace openlr
