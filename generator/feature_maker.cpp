#include "generator/feature_maker.hpp"

#include "generator/holes.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"


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

  helper.GetOuter().ForEachArea(true /* collectID */, [&](auto && pts, auto && ids)
  {
    createFB(std::move(pts), ids);
  });

  return size != m_queue.size();
}


FeatureMaker::FeatureMaker(IDRInterfacePtr const & cache)
  : FeatureMakerSimple(cache)
{
  m_placeClass = classif().GetTypeByPath({"place"});
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

bool FeatureMaker::BuildFromRelation(OsmElement & p, FeatureBuilderParams const & params)
{
  uint32_t const placeType = params.FindType(m_placeClass, 1);
  if (placeType != ftype::GetEmptyValue())
  {
    std::optional<m2::PointD> center;
    auto const nodes = osm_element::GetPlaceNodeFromMembers(p);
    if (!nodes.empty())
    {
      // Patch to avoid multiple instances from Node and Relation. Node should inherit all needed tags (population).
      // This is ok, because "canonical" OSM assumes that country/state place tag presents in Nodes only.
      /// @todo Store boundaries like for cities?
      if (ftypes::IsCountryChecker::Instance()(placeType) || ftypes::IsStateChecker::Instance()(placeType))
        return false;

      // admin_centre will be the first
      center = ReadNode(nodes.front());
    }
    else
    {
      // Calculate center of the biggest outer polygon.
      HolesRelation helper(m_cache);
      helper.Build(&p);

      double maxArea = 0;
      helper.GetOuter().ForEachArea(false /* collectID */, [&](auto && pts, auto &&)
      {
        m2::RectD rect;
        CalcRect(pts, rect);
        double const currArea = rect.Area();
        if (currArea > maxArea)
        {
          center = FeatureBuilder::GetGeometryCenter(pts);
          maxArea = currArea;
        }
      });
    }

    if (!center)
      return false;

    // Make separate place Point FeatureFuilder.
    FeatureBuilder fb;
    fb.SetCenter(*center);

    fb.SetOsmId(base::MakeOsmRelation(p.m_id));

    FeatureBuilderParams copyParams(params);
    copyParams.SetType(placeType);
    fb.SetParams(copyParams);

    m_queue.push(std::move(fb));

    // Default processing without place type.
    copyParams = params;
    copyParams.PopExactType(placeType);
    if (copyParams.FinishAddingTypes())
      FeatureMakerSimple::BuildFromRelation(p, copyParams);

    return true;
  }

  return FeatureMakerSimple::BuildFromRelation(p, params);
}

}  // namespace generator
