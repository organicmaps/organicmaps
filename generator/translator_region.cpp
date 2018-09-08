#include "generator/translator_region.hpp"

#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/holes.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/region_info_collector.hpp"

#include "indexer/classificator.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <set>
#include <string>
#include <vector>

namespace generator
{
TranslatorRegion::TranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                                   cache::IntermediateDataReader & holder,
                                   RegionInfoCollector & regionInfoCollector)
  : m_emitter(emitter),
    m_holder(holder),
    m_regionInfoCollector(regionInfoCollector)
{
}

void TranslatorRegion::EmitElement(OsmElement * p)
{
  CHECK(p, ("Tried to emit a null OsmElement"));

  FeatureParams params;
  if (!(IsSuitableElement(p) && ParseParams(p, params)))
    return;

  switch (p->type)
  {
  case OsmElement::EntityType::Relation:
  {
    BuildFeatureAndEmitFromRelation(p, params);
    break;
  }
  case OsmElement::EntityType::Way:
  {
    BuildFeatureAndEmitFromWay(p, params);
    break;
  }
  default:
    break;
  }
}

bool TranslatorRegion::IsSuitableElement(OsmElement const * p) const
{
  static std::set<std::string> const adminLevels = {"2", "4", "5", "6", "7", "8"};
  static std::set<std::string> const places = {"city", "town", "village", "suburb", "neighbourhood",
                                               "hamlet", "locality", "isolated_dwelling"};

  bool haveBoundary = false;
  bool haveAdminLevel = false;
  bool haveName = false;
  for (auto const & t : p->Tags())
  {
    if (t.key == "place" && places.find(t.value) != places.end())
      return true;

    if (t.key == "boundary" && t.value == "administrative")
      haveBoundary = true;
    else if (t.key == "admin_level" && adminLevels.find(t.value) != adminLevels.end())
      haveAdminLevel = true;
    else if (t.key == "name" && !t.value.empty())
      haveName = true;

    if (haveBoundary && haveAdminLevel && haveName)
      return true;
  }

  return false;
}

void TranslatorRegion::AddInfoAboutRegion(OsmElement const * p, base::GeoObjectId osmId) const
{
  m_regionInfoCollector.Add(osmId, *p);
}

bool TranslatorRegion::ParseParams(OsmElement * p, FeatureParams & params) const
{
  ftype::GetNameAndType(p, params, [] (uint32_t type) {
    return classif().IsTypeValid(type);
  });
  return params.IsValid();
}

void TranslatorRegion::BuildFeatureAndEmitFromRelation(OsmElement const * p, FeatureParams & params)
{
  HolesRelation helper(m_holder);
  helper.Build(p);
  auto const & holesGeometry = helper.GetHoles();
  auto & outer = helper.GetOuter();
  outer.ForEachArea(true, [&] (FeatureBuilder1::PointSeq const & pts,
                    std::vector<uint64_t> const & ids)
  {
    FeatureBuilder1 fb;
    for (uint64_t id : ids)
      fb.AddOsmId(base::MakeOsmWay(id));

    for (auto const & pt : pts)
      fb.AddPoint(pt);

    auto const id = base::MakeOsmRelation(p->id);
    fb.AddOsmId(id);
    if (!fb.IsGeometryClosed())
      return;

    fb.SetAreaAddHoles(holesGeometry);
    fb.SetParams(params);
    AddInfoAboutRegion(p, id);
    (*m_emitter)(fb);
  });
}

void TranslatorRegion::BuildFeatureAndEmitFromWay(OsmElement const * p, FeatureParams & params)
{
  FeatureBuilder1 fb;
  m2::PointD pt;
  for (uint64_t ref : p->Nodes())
  {
    if (!m_holder.GetNode(ref, pt.y, pt.x))
      return;

    fb.AddPoint(pt);
  }

  auto const id = base::MakeOsmWay(p->id);
  fb.SetOsmId(id);
  fb.SetParams(params);
  if (!fb.IsGeometryClosed())
    return;

  fb.SetArea();
  AddInfoAboutRegion(p, id);
  (*m_emitter)(fb);
}
}  // namespace generator
