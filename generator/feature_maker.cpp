#include "generator/feature_maker.hpp"

#include "generator/holes.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"

#include "geometry/mercator.hpp"
#include "geometry/polygon.hpp"


namespace generator
{
using namespace feature;

std::shared_ptr<FeatureMakerBase> FeatureMakerSimple::Clone() const
{
  return std::make_shared<FeatureMakerSimple>();
}

void FeatureMakerSimple::ParseParams(FeatureBuilderParams & params, OsmElement & p) const
{
  auto const & cl = classif();
  ftype::GetNameAndType(&p, params,
                        [&cl] (uint32_t type) { return cl.IsTypeValid(type); },
                        [this](OsmElement const * p) { return GetOrigin(*p); });
}

std::optional<m2::PointD> FeatureMakerSimple::ReadNode(uint64_t id) const
{
  m2::PointD res;
  if (m_cache->GetNode(id, res.y, res.x))
    return res;
  return {};
}

std::optional<m2::PointD> FeatureMakerSimple::GetOrigin(OsmElement const & e) const
{
  if (e.IsNode())
  {
    CHECK(e.m_lat != 0 || e.m_lon != 0, (e.m_id));
    return mercator::FromLatLon(e.m_lat, e.m_lon);
  }
  else if (e.IsWay())
  {
    CHECK(!e.m_nodes.empty(), (e.m_id));
    return ReadNode(e.m_nodes.front());
  }
  else
  {
    CHECK(!e.m_members.empty(), (e.m_id));
    for (auto const & m : e.m_members)
    {
      if (m.m_type == OsmElement::EntityType::Node)
        return ReadNode(m.m_ref);
    }

    for (auto const & m : e.m_members)
    {
      if (m.m_type == OsmElement::EntityType::Way)
      {
        WayElement way(m.m_ref);
        if (m_cache->GetWay(m.m_ref, way))
        {
          CHECK(!way.m_nodes.empty(), (m.m_ref));
          return ReadNode(way.m_nodes.front());
        }
      }
    }

    LOG(LWARNING, ("No geometry members for relation", e.m_id));
    return {};
  }
}

bool FeatureMakerSimple::BuildFromNode(OsmElement & p, FeatureBuilderParams const & params)
{
  FeatureBuilder fb;
  fb.SetCenter(mercator::FromLatLon(p.m_lat, p.m_lon));

  fb.SetOsmId(base::MakeOsmNode(p.m_id));
  fb.SetParams(params);

  m_queue.push(std::move(fb));
  return true;
}

bool FeatureMakerSimple::BuildFromWay(OsmElement & p, FeatureBuilderParams const & params)
{
  auto const & nodes = p.Nodes();
  if (nodes.size() < 2)
    return false;

  FeatureBuilder fb;

  std::vector<m2::PointD> points;
  points.reserve(nodes.size());
  for (uint64_t ref : nodes)
  {
    m2::PointD pt;
    if (!m_cache->GetNode(ref, pt.y, pt.x))
      return false;
    points.push_back(pt);
  }
  fb.AssignPoints(std::move(points));

  fb.SetOsmId(base::MakeOsmWay(p.m_id));
  fb.SetParams(params);

  if (fb.IsGeometryClosed())
    fb.SetArea();
  else
    fb.SetLinear(params.GetReversedGeometry());

  m_queue.push(std::move(fb));
  return true;
}

bool FeatureMakerSimple::BuildFromRelation(OsmElement & p, FeatureBuilderParams const & params)
{
  HolesRelation helper(m_cache);
  helper.Build(&p);
  auto const & holesGeometry = helper.GetHoles();
  auto const size = m_queue.size();

  auto const createFB = [&](auto && pts, auto const & ids)
  {
    FeatureBuilder fb;

    fb.AssignArea(std::move(pts), holesGeometry);
    CHECK(fb.IsGeometryClosed(), ());

    for (uint64_t id : ids)
      fb.AddOsmId(base::MakeOsmWay(id));
    fb.AddOsmId(base::MakeOsmRelation(p.m_id));

    fb.SetParams(params);
    fb.SetArea();

    m_queue.push(std::move(fb));
  };

  bool const maxAreaMode = IsMaxRelationAreaMode(params);
  double maxArea = -1.0;
  FeatureBuilder::PointSeq maxPoints;
  std::vector<uint64_t> maxIDs;

  helper.GetOuter().ForEachArea(true /* collectID */, [&](auto && pts, auto && ids)
  {
    if (maxAreaMode)
    {
      double const area = GetPolygonArea(pts.begin(), pts.end());
      if (area > maxArea)
      {
        maxArea = area;
        maxPoints = std::move(pts);
        maxIDs = std::move(ids);
      }
    }
    else
      createFB(std::move(pts), ids);
  });

  if (maxAreaMode && maxArea > 0)
    createFB(std::move(maxPoints), maxIDs);

  return size != m_queue.size();
}

std::shared_ptr<FeatureMakerBase> FeatureMaker::Clone() const
{
  return std::make_shared<FeatureMaker>();
}

void FeatureMaker::ParseParams(FeatureBuilderParams & params, OsmElement & p) const
{
  ftype::GetNameAndType(&p, params, &feature::IsUsefulType,
                        [this](OsmElement const * p) { return GetOrigin(*p); });
}

bool FeatureMaker::IsMaxRelationAreaMode(FeatureBuilderParams const & params) const
{
  auto const & c = classif();
  bool const emptyName = params.IsEmptyNames();

  // Select _one_ max area polygon, if we have Point drawing rules only (like place=*).
  for (uint32_t const t : params.m_types)
  {
    auto const * obj = c.GetObject(t);
    if (obj->IsDrawableLike(GeomType::Line, emptyName) || obj->IsDrawableLike(GeomType::Area, emptyName))
      return false;
  }
  return true;
}

}  // namespace generator
