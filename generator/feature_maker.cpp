#include "generator/feature_maker.hpp"

#include "generator/holes.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <utility>

using namespace feature;

namespace generator
{
std::shared_ptr<FeatureMakerBase> FeatureMakerSimple::Clone() const
{
  return std::make_shared<FeatureMakerSimple>();
}

void FeatureMakerSimple::ParseParams(FeatureParams & params, OsmElement & p) const
{
  ftype::GetNameAndType(&p, params, [] (uint32_t type) {
    return classif().IsTypeValid(type);
  });
}

bool FeatureMakerSimple::BuildFromNode(OsmElement & p, FeatureParams const & params)
{
  FeatureBuilder fb;
  fb.SetCenter(MercatorBounds::FromLatLon(p.m_lat, p.m_lon));
  fb.SetOsmId(base::MakeOsmNode(p.m_id));
  fb.SetParams(params);
  m_queue.push(std::move(fb));
  return true;
}

bool FeatureMakerSimple::BuildFromWay(OsmElement & p, FeatureParams const & params)
{
  auto const & cache = m_cache->GetCache();
  auto const & nodes = p.Nodes();
  if (nodes.size() < 2)
    return false;

  FeatureBuilder fb;
  m2::PointD pt;
  for (uint64_t ref : nodes)
  {
    if (!cache->GetNode(ref, pt.y, pt.x))
      return false;

    fb.AddPoint(pt);
  }

  fb.SetOsmId(base::MakeOsmWay(p.m_id));
  fb.SetParams(params);
  if (fb.IsGeometryClosed())
  {
    HolesProcessor processor(p.m_id, cache);
    cache->ForEachRelationByWay(p.m_id, processor);
    fb.SetHoles(processor.GetHoles());
    fb.SetArea();
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
  auto const & cache = m_cache->GetCache();
  HolesRelation helper(cache);
  helper.Build(&p);
  auto const & holesGeometry = helper.GetHoles();
  auto & outer = helper.GetOuter();
  auto const size = m_queue.size();
  auto const func = [&](FeatureBuilder::PointSeq const & pts, std::vector<uint64_t> const & ids)
  {
    FeatureBuilder fb;
    for (uint64_t id : ids)
      fb.AddOsmId(base::MakeOsmWay(id));

    for (auto const & pt : pts)
      fb.AddPoint(pt);

    fb.AddOsmId(base::MakeOsmRelation(p.m_id));
    if (!fb.IsGeometryClosed())
      return;

    fb.SetHoles(holesGeometry);
    fb.SetParams(params);
    fb.SetArea();
    m_queue.push(std::move(fb));
  };

  outer.ForEachArea(true /* collectID */, func);
  return size != m_queue.size();
}

std::shared_ptr<FeatureMakerBase> FeatureMaker::Clone() const
{
  return std::make_shared<FeatureMaker>();
}

void FeatureMaker::ParseParams(FeatureParams & params, OsmElement & p) const
{
  ftype::GetNameAndType(&p, params);
}
}  // namespace generator
