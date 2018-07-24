#include "openlr/road_info_getter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/data_source.hpp"

#include "base/assert.hpp"

#include "std/iterator.hpp"

namespace openlr
{
RoadInfoGetter::RoadInfoGetter(DataSource const & dataSource)
  : m_dataSource(dataSource), m_c(classif())
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
  info.m_frc = GetFunctionalRoadClass(feature::TypesHolder(ft));
  info.m_fow = GetFormOfWay(feature::TypesHolder(ft));
  it = m_cache.emplace(fid, info).first;

  return it->second;
}

FunctionalRoadClass RoadInfoGetter::GetFunctionalRoadClass(feature::TypesHolder const & types) const
{
  if (m_trunkChecker(types))
    return FunctionalRoadClass::FRC0;

  if (m_primaryChecker(types))
    return FunctionalRoadClass::FRC1;

  if (m_secondaryChecker(types))
    return FunctionalRoadClass::FRC2;

  if (m_tertiaryChecker(types))
    return FunctionalRoadClass::FRC3;

  if (m_residentialChecker(types))
    return FunctionalRoadClass::FRC4;

  if (m_livingStreetChecker(types))
    return FunctionalRoadClass::FRC5;

  return FunctionalRoadClass::FRC7;
}

FormOfWay RoadInfoGetter::GetFormOfWay(feature::TypesHolder const & types) const
{
  if (m_trunkChecker(types))
    return FormOfWay::Motorway;

  if (m_primaryChecker(types))
    return FormOfWay::MultipleCarriageway;

  return FormOfWay::SingleCarriageway;
}
}  // namespace openlr
