#include "generator/translator_region.hpp"

#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/holes.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"

#include <set>
#include <string>
#include <vector>

namespace generator
{
TranslatorRegion::TranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                                   cache::IntermediateDataReader & holder) :
  m_emitter(emitter),
  m_holder(holder)
{
}

void TranslatorRegion::EmitElement(OsmElement * p)
{
  CHECK(p, ("Tried to emit a null OsmElement"));

  switch(p->type)
  {
  case OsmElement::EntityType::Relation:
  {
    FeatureParams params;
    if (!(IsSuitableElement(p) && ParseParams(p, params)))
      return;

    BuildFeatureAndEmit(p, params);
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

void TranslatorRegion::AddInfoAboutRegion(OsmElement const * p,
                                          FeatureBuilder1 & ft) const
{
}

bool TranslatorRegion::ParseParams(OsmElement * p, FeatureParams & params) const
{
  ftype::GetNameAndType(p, params, [] (uint32_t type) {
    return classif().IsTypeValid(type);
  });
  return params.IsValid();
}

void TranslatorRegion::BuildFeatureAndEmit(OsmElement const * p, FeatureParams & params)
{
  HolesRelation helper(m_holder);
  helper.Build(p);
  auto const & holesGeometry = helper.GetHoles();
  auto & outer = helper.GetOuter();
  outer.ForEachArea(true, [&] (FeatureBuilder1::PointSeq const & pts,
                    std::vector<uint64_t> const & ids)
  {
    FeatureBuilder1 ft;
    for (uint64_t id : ids)
      ft.AddOsmId(osm::Id::Way(id));

    for (auto const & pt : pts)
      ft.AddPoint(pt);

    ft.AddOsmId(osm::Id::Relation(p->id));
    if (!ft.IsGeometryClosed())
      return;

    ft.SetAreaAddHoles(holesGeometry);
    ft.SetParams(params);
    AddInfoAboutRegion(p, ft);
    (*m_emitter)(ft);
  });
}
}  // namespace generator
