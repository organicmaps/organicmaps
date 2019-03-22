#include "generator/feature_maker.hpp"

#include "generator/holes.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <utility>

namespace generator
{
void FeatureMakerSimple::ParseParams(FeatureParams & params, OsmElement & p) const
{
  ftype::GetNameAndType(&p, params, [] (uint32_t type) {
    return classif().IsTypeValid(type);
  });
}

bool FeatureMakerSimple::BuildFromNode(OsmElement & p, FeatureParams const & params)
{
  FeatureBuilder1 fb;
  fb.SetCenter(MercatorBounds::FromLatLon(p.lat, p.lon));
  fb.SetOsmId(base::MakeOsmNode(p.id));
  fb.SetParams(params);
  m_queue.push(std::move(fb));
  return true;
}

bool FeatureMakerSimple::BuildFromWay(OsmElement & p, FeatureParams const & params)
{
  auto const & nodes = p.Nodes();
  if (nodes.size() < 2)
    return false;

  FeatureBuilder1 fb;
  m2::PointD pt;
  for (uint64_t ref : nodes)
  {
    if (!m_holder.GetNode(ref, pt.y, pt.x))
      return false;

    fb.AddPoint(pt);
  }

  fb.SetOsmId(base::MakeOsmWay(p.id));
  fb.SetParams(params);
  if (fb.IsGeometryClosed())
  {
    HolesProcessor processor(p.id, m_holder);
    m_holder.ForEachRelationByWay(p.id, processor);
    fb.SetAreaAddHoles(processor.GetHoles());
  }
  else
  {
    fb.SetLinear(params.m_reverseGeometry);
  }

  m_queue.push(std::move(fb));
  return true;
}

bool FeatureMakerSimple::BuildFromRelation(OsmElement & p, FeatureParams const & params)
{
  HolesRelation helper(m_holder);
  helper.Build(&p);
  auto const & holesGeometry = helper.GetHoles();
  auto & outer = helper.GetOuter();
  auto const size = m_queue.size();
  auto const func = [&](FeatureBuilder1::PointSeq const & pts, std::vector<uint64_t> const & ids)
  {
    FeatureBuilder1 fb;
    for (uint64_t id : ids)
      fb.AddOsmId(base::MakeOsmWay(id));

    for (auto const & pt : pts)
      fb.AddPoint(pt);

    fb.AddOsmId(base::MakeOsmRelation(p.id));
    if (!fb.IsGeometryClosed())
      return;

    fb.SetAreaAddHoles(holesGeometry);
    fb.SetParams(params);
    m_queue.push(std::move(fb));
  };

  outer.ForEachArea(true /* collectID */, func);
  return size != m_queue.size();
}

void FeatureMaker::ParseParams(FeatureParams & params, OsmElement & p) const
{
  ftype::GetNameAndType(&p, params);
}
}  // namespace generator
