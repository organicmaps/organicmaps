#include "generator/translator_geocoder_base.hpp"

#include "generator/collector_interface.hpp"
#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"
#include "generator/holes.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/regions/collector_region_info.hpp"

#include "indexer/classificator.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <set>

namespace generator
{
TranslatorGeocoderBase::TranslatorGeocoderBase(std::shared_ptr<EmitterInterface> emitter,
                                               cache::IntermediateDataReader & holder)
  : m_emitter(emitter), m_holder(holder) {}

void TranslatorGeocoderBase::EmitElement(OsmElement * p)
{
  CHECK(p, ("Tried to emit a null OsmElement"));

  FeatureParams params;
  // We are forced to copy because ParseParams() removes tags from OsmElement.
  OsmElement osmElement = *p;
  if (!(IsSuitableElement(p) && ParseParams(&osmElement, params)))
    return;

  switch (p->type)
  {
  case OsmElement::EntityType::Node:
    BuildFeatureAndEmitFromNode(p, params);
    break;
  case OsmElement::EntityType::Relation:
    BuildFeatureAndEmitFromRelation(p, params);
    break;
  case OsmElement::EntityType::Way:
    BuildFeatureAndEmitFromWay(p, params);
    break;
  default:
    break;
  }
}

bool TranslatorGeocoderBase::Finish()
{
  for (auto & collector : m_collectors)
    collector->Save();

  return m_emitter->Finish();
}

void TranslatorGeocoderBase::GetNames(std::vector<std::string> & names) const
{
  m_emitter->GetNames(names);
}

void TranslatorGeocoderBase::AddCollector(std::shared_ptr<CollectorInterface> collector)
{
  m_collectors.push_back(collector);
}

bool TranslatorGeocoderBase::ParseParams(OsmElement * p, FeatureParams & params) const
{
  ftype::GetNameAndType(p, params, [] (uint32_t type) {
    return classif().IsTypeValid(type);
  });
  return params.IsValid();
}

void TranslatorGeocoderBase::Emit(FeatureBuilder1 & fb, OsmElement const * p)
{
  auto const id = fb.GetMostGenericOsmId();
  for (auto const & collector : m_collectors)
    collector->Collect(id, *p);

  auto unused = fb.PreSerialize();
  UNUSED_VALUE(unused);

  (*m_emitter)(fb);
}

void TranslatorGeocoderBase::BuildFeatureAndEmitFromRelation(OsmElement const * p,
                                                             FeatureParams const & params)
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

    fb.AddOsmId(base::MakeOsmRelation(p->id));
    if (!fb.IsGeometryClosed())
      return;

    fb.SetAreaAddHoles(holesGeometry);
    fb.SetParams(params);
    Emit(fb, p);
  });
}

void TranslatorGeocoderBase::BuildFeatureAndEmitFromWay(OsmElement const * p,
                                                        FeatureParams const & params)
{
  FeatureBuilder1 fb;
  m2::PointD pt;
  for (uint64_t ref : p->Nodes())
  {
    if (!m_holder.GetNode(ref, pt.y, pt.x))
      return;

    fb.AddPoint(pt);
  }

  fb.SetOsmId(base::MakeOsmWay(p->id));
  fb.SetParams(params);
  if (!fb.IsGeometryClosed())
    return;

  fb.SetArea();
  Emit(fb, p);
}

void TranslatorGeocoderBase::BuildFeatureAndEmitFromNode(OsmElement const * p,
                                                         FeatureParams const & params)
{
  m2::PointD const pt = MercatorBounds::FromLatLon(p->lat, p->lon);
  FeatureBuilder1 fb;
  fb.SetCenter(pt);
  fb.SetOsmId(base::MakeOsmNode(p->id));
  fb.SetParams(params);
  Emit(fb, p);
}
}  // namespace generator
