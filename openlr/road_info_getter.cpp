#include "openlr/road_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_source.hpp"

#include "base/assert.hpp"

namespace
{
openlr::FunctionalRoadClass HighwayClassToFunctionalRoadClass(ftypes::HighwayClass const & hwClass)
{
  switch (hwClass)
  {
  case ftypes::HighwayClass::Trunk: return openlr::FunctionalRoadClass::FRC0;
  case ftypes::HighwayClass::Primary: return openlr::FunctionalRoadClass::FRC1;
  case ftypes::HighwayClass::Secondary: return openlr::FunctionalRoadClass::FRC2;
  case ftypes::HighwayClass::Tertiary: return openlr::FunctionalRoadClass::FRC3;
  case ftypes::HighwayClass::LivingStreet: return openlr::FunctionalRoadClass::FRC4;
  case ftypes::HighwayClass::Service: return openlr::FunctionalRoadClass::FRC5;
  default: return openlr::FunctionalRoadClass::FRC7;
  }
}
}  // namespace

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
  info.m_fow = GetFormOfWay(feature::TypesHolder(ft));
  info.m_hwClass = ftypes::GetHighwayClass(feature::TypesHolder(ft));
  info.m_link = ftypes::IsLinkChecker::Instance()(ft);
  info.m_oneWay = ftypes::IsOneWayChecker::Instance()(ft);
  info.m_frc = HighwayClassToFunctionalRoadClass(info.m_hwClass);

  it = m_cache.emplace(fid, info).first;

  return it->second;
}

FormOfWay RoadInfoGetter::GetFormOfWay(feature::TypesHolder const & types) const
{
  auto const hwClass = ftypes::GetHighwayClass(feature::TypesHolder(types));
  if (hwClass == ftypes::HighwayClass::Trunk)
    return FormOfWay::Motorway;

  if (hwClass == ftypes::HighwayClass::Primary)
    return FormOfWay::MultipleCarriageway;

  return FormOfWay::SingleCarriageway;
}
}  // namespace openlr
