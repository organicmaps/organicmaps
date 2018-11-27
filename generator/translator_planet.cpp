#include "generator/translator_planet.hpp"

#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/holes.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <string>
#include <vector>

namespace generator
{
TranslatorPlanet::TranslatorPlanet(std::shared_ptr<EmitterInterface> emitter,
                                   cache::IntermediateDataReader & holder,
                                   feature::GenerateInfo const & info)
  : m_emitter(emitter)
  , m_cache(holder)
  , m_coastType(info.m_makeCoasts ? classif().GetCoastType() : 0)
  , m_routingTagsProcessor(info.GetIntermediateFileName(MAXSPEEDS_FILENAME))
  , m_nodeRelations(m_routingTagsProcessor)
  , m_wayRelations(m_routingTagsProcessor)
  , m_metalinesBuilder(info.GetIntermediateFileName(METALINES_FILENAME))
{
  auto const addrFilePath = info.GetAddressesFileName();
  if (!addrFilePath.empty())
    m_addrWriter.reset(new FileWriter(addrFilePath));

  auto const restrictionsFilePath = info.GetIntermediateFileName(RESTRICTIONS_FILENAME);
  if (!restrictionsFilePath.empty())
    m_routingTagsProcessor.m_restrictionWriter.Open(restrictionsFilePath);

  auto const roadAccessFilePath = info.GetIntermediateFileName(ROAD_ACCESS_FILENAME);
  if (!roadAccessFilePath.empty())
    m_routingTagsProcessor.m_roadAccessWriter.Open(roadAccessFilePath);

  auto const camerasToWaysFilePath = info.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME);
  auto const camerasNodesToWaysFilePath = info.GetIntermediateFileName(CAMERAS_NODES_TO_WAYS_FILE);
  auto const camerasMaxSpeedFilePath = info.GetIntermediateFileName(CAMERAS_MAXSPEED_FILE);
  if (!camerasToWaysFilePath.empty() && !camerasNodesToWaysFilePath.empty() &&
      !camerasMaxSpeedFilePath.empty())
  {
    m_routingTagsProcessor.m_cameraNodeWriter.Open(camerasToWaysFilePath, camerasNodesToWaysFilePath,
                                                   camerasMaxSpeedFilePath);
  }
}

void TranslatorPlanet::EmitElement(OsmElement * p)
{
  CHECK(p, ("Tried to emit a null OsmElement"));

  FeatureParams params;
  switch (p->type)
  {
  case OsmElement::EntityType::Node:
  {
    if (p->m_tags.empty())
      break;

    if (!ParseType(p, params))
      break;

    m2::PointD const pt = MercatorBounds::FromLatLon(p->lat, p->lon);
    EmitPoint(pt, params, base::MakeOsmNode(p->id));
    break;
  }
  case OsmElement::EntityType::Way:
  {
    FeatureBuilder1 ft;
    m2::PointD pt;
    // Parse geometry.
    for (uint64_t ref : p->Nodes())
    {
      if (!m_cache.GetNode(ref, pt.y, pt.x))
        break;
      ft.AddPoint(pt);
    }

    if (p->Nodes().size() != ft.GetPointsCount())
      break;

    if (ft.GetPointsCount() < 2)
      break;

    if (!ParseType(p, params))
      break;

    ft.SetOsmId(base::MakeOsmWay(p->id));

    m_routingTagsProcessor.m_maxspeedsCollector.Process(*p);

    bool isCoastline = (m_coastType != 0 && params.IsTypeExist(m_coastType));

    EmitArea(ft, params, [&] (FeatureBuilder1 & ft)
    {
      // Emit coastline feature only once.
      isCoastline = false;
      HolesProcessor processor(p->id, m_cache);
      m_cache.ForEachRelationByWay(p->id, processor);
      ft.SetAreaAddHoles(processor.GetHoles());
    });

    m_metalinesBuilder(*p, params);
    EmitLine(ft, params, isCoastline);
    break;
  }
  case OsmElement::EntityType::Relation:
  {
    if (!p->HasTagValue("type", "multipolygon") && !p->HasTagValue("type", "boundary"))
      break;

    if (!ParseType(p, params))
      break;

    HolesRelation helper(m_cache);
    helper.Build(p);

    auto const & holesGeometry = helper.GetHoles();
    auto & outer = helper.GetOuter();

    outer.ForEachArea(true /* collectID */, [&] (FeatureBuilder1::PointSeq const & pts,
                      std::vector<uint64_t> const & ids)
    {
      FeatureBuilder1 ft;

      for (uint64_t id : ids)
        ft.AddOsmId(base::MakeOsmWay(id));

      for (auto const & pt : pts)
        ft.AddPoint(pt);

      ft.AddOsmId(base::MakeOsmRelation(p->id));
      EmitArea(ft, params, [&holesGeometry] (FeatureBuilder1 & ft) {
        ft.SetAreaAddHoles(holesGeometry);
      });
    });
    break;
  }
  default:
    break;
  }
}

bool TranslatorPlanet::Finish()
{
  return m_emitter->Finish();
}

void TranslatorPlanet::GetNames(std::vector<std::string> & names) const
{
  m_emitter->GetNames(names);
}

bool TranslatorPlanet::ParseType(OsmElement * p, FeatureParams & params)
{
  // Get tags from parent relations.
  if (p->IsNode())
  {
    m_nodeRelations.Reset(p->id, p);
    m_cache.ForEachRelationByNodeCached(p->id, m_nodeRelations);
  }
  else if (p->IsWay())
  {
    m_wayRelations.Reset(p->id, p);
    m_cache.ForEachRelationByWayCached(p->id, m_wayRelations);
  }

  // Get params from element tags.
  ftype::GetNameAndType(p, params);
  if (!params.IsValid())
    return false;

  m_routingTagsProcessor.m_cameraNodeWriter.Process(*p, params, m_cache);
  m_routingTagsProcessor.m_roadAccessWriter.Process(*p);

  return true;
}

void TranslatorPlanet::EmitPoint(m2::PointD const & pt, FeatureParams params,
                                 base::GeoObjectId id) const
{
  if (!feature::RemoveUselessTypes(params.m_types, feature::GEOM_POINT))
    return;

  FeatureBuilder1 ft;
  ft.SetCenter(pt);
  ft.SetOsmId(id);
  EmitFeatureBase(ft, params);
}

void TranslatorPlanet::EmitLine(FeatureBuilder1 & ft, FeatureParams params, bool isCoastLine) const
{
  if (!isCoastLine && !feature::RemoveUselessTypes(params.m_types, feature::GEOM_LINE))
    return;

  ft.SetLinear(params.m_reverseGeometry);
  EmitFeatureBase(ft, params);
}

void TranslatorPlanet::EmitArea(FeatureBuilder1 & ft, FeatureParams params,
                                std::function<void(FeatureBuilder1 &)> fn)
{
  using namespace feature;

  // Ensure that we have closed area geometry.
  if (!ft.IsGeometryClosed())
    return;

  // @TODO(bykoianko) Now if a feature (a relation in most cases and a way in a rare case) has tag
  // place with a value city, town, village or hamlet (IsCityTownOrVillage() returns true) city boundary
  // will be emitted. It's correct according to osm documentation but very often city boundaries
  // are mapped in another way. City boundary may be mapped with a relation of a few lines and
  // one point (town of city name). And only this point has tag place with an appropriate value.
  // For example almost all cities and towns in Poland are mapped this way. It's wrong to consider
  // such relations as a city boundaries directly because sometimes it may cover much bigger area.
  // But to use information about such city boundaries some investigation should be made.
  if (ftypes::IsCityTownOrVillage(params.m_types))
  {
    auto fb = ft;
    fn(fb);
    m_emitter->EmitCityBoundary(fb, params);
  }

  // Key point here is that IsDrawableLike and RemoveUselessTypes
  // work a bit differently for GEOM_AREA.
  if (IsDrawableLike(params.m_types, GEOM_AREA))
  {
    // Make the area feature if it has unique area styles.
    VERIFY(RemoveUselessTypes(params.m_types, GEOM_AREA), (params));
    fn(ft);
    EmitFeatureBase(ft, params);
  }
  else
  {
    // Try to make the point feature if it has point styles.
    EmitPoint(ft.GetGeometryCenter(), params, ft.GetLastOsmId());
  }
}

void TranslatorPlanet::EmitFeatureBase(FeatureBuilder1 & ft,
                                       FeatureParams const & params) const
{
  ft.SetParams(params);
  if (!ft.PreSerializeAndRemoveUselessNames())
    return;

  std::string addr;
  if (m_addrWriter &&
      ftypes::IsBuildingChecker::Instance()(params.m_types) &&
      ft.FormatFullAddress(addr))
  {
    m_addrWriter->Write(addr.c_str(), addr.size());
  }

  (*m_emitter)(ft);
}
}  // namespace generator
