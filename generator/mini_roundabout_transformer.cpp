#include "generator/mini_roundabout_transformer.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <unordered_map>

namespace generator
{
namespace
{
double constexpr kDefaultRadiusMeters = 5.0;

/// \brief Moves |it| on the |road| range one step from current |it| element.
bool MoveIterAwayFromRoundabout(feature::FeatureBuilder::PointSeq::iterator & it,
                                feature::FeatureBuilder::PointSeq const & road)
{
  bool middlePoint = false;
  if (it == road.begin())
  {
    ++it;
  }
  else if (it + 1 == road.end())
  {
    --it;
  }
  else
  {
    // Mini-roundabout is on the middle of the road so we need to insert 2 points to the roundabout
    // circle and create additional surrogate road.
    --it;
    middlePoint = true;
  }
  return middlePoint;
}

void UpdateFeatureGeometry(feature::FeatureBuilder::PointSeq const & seq,
                           feature::FeatureBuilder & fb)
{
  fb.ResetGeometry();
  for (auto const & p : seq)
    fb.AddPoint(p);
}

feature::FeatureBuilder::PointSeq::iterator GetIterOnRoad(m2::PointD const & point,
                                                          feature::FeatureBuilder::PointSeq & road)
{
  return std::find_if(road.begin(), road.end(), [&point](m2::PointD const & pointOnRoad)
  {
    return m2::AlmostEqualAbs(pointOnRoad, point, kMwmPointAccuracy);
  });
}
} // namespace

MiniRoundaboutData::MiniRoundaboutData(std::vector<MiniRoundaboutInfo> && data)
  : m_data(std::move(data))
{
   for (auto const & d : m_data)
     m_ways.insert(std::end(m_ways), std::cbegin(d.m_ways), std::cend(d.m_ways));

   base::SortUnique(m_ways);
}

bool MiniRoundaboutData::RoadExists(feature::FeatureBuilder const & fb) const
{
  return std::binary_search(std::cbegin(m_ways), std::cend(m_ways),
                            fb.GetMostGenericOsmId().GetSerialId());
}

std::vector<MiniRoundaboutInfo> const & MiniRoundaboutData::GetData() const
{
  return m_data;
}

MiniRoundaboutData ReadDataMiniRoundabout(std::string const & intermediateFilePath)
{
  auto roundabouts = ReadMiniRoundabouts(intermediateFilePath);
  LOG(LINFO, ("Loaded", roundabouts.size(), "mini_roundabouts from file", intermediateFilePath));
  return MiniRoundaboutData(std::move(roundabouts));
}

MiniRoundaboutTransformer::MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                                                     feature::AffiliationInterface const & affiliation)
  : MiniRoundaboutTransformer(data, affiliation, kDefaultRadiusMeters)
{
}

MiniRoundaboutTransformer::MiniRoundaboutTransformer(std::vector<MiniRoundaboutInfo> const & data,
                                                     feature::AffiliationInterface const & affiliation,
                                                     double radiusMeters)
  : m_roundabouts(data)
  , m_radiusMercator(mercator::MetersToMercator(radiusMeters))
  , m_affiliation(&affiliation)
{
}

void MiniRoundaboutTransformer::UpdateRoadType(FeatureParams::Types const & foundTypes,
                                               uint32_t & roadType)
{
  // Highways are sorted from the most to least important.
  static std::array<uint32_t, 14> const kHighwayTypes = {
      classif().GetTypeByPath({"highway", "motorway"}),
      classif().GetTypeByPath({"highway", "motorway_link"}),
      classif().GetTypeByPath({"highway", "trunk"}),
      classif().GetTypeByPath({"highway", "trunk_link"}),
      classif().GetTypeByPath({"highway", "primary"}),
      classif().GetTypeByPath({"highway", "primary_link"}),
      classif().GetTypeByPath({"highway", "secondary"}),
      classif().GetTypeByPath({"highway", "secondary_link"}),
      classif().GetTypeByPath({"highway", "tertiary"}),
      classif().GetTypeByPath({"highway", "tertiary_link"}),
      classif().GetTypeByPath({"highway", "unclassified"}),
      classif().GetTypeByPath({"highway", "residential"}),
      classif().GetTypeByPath({"highway", "living_street"}),
      classif().GetTypeByPath({"highway", "service"})};

  for (uint32_t t : foundTypes)
  {
    auto const it = std::find(kHighwayTypes.begin(), kHighwayTypes.end(), t);
    if (it == kHighwayTypes.end())
      continue;

    auto const itPrev = std::find(kHighwayTypes.begin(), kHighwayTypes.end(), roadType);
    if (itPrev == kHighwayTypes.end() || itPrev > it)
      roadType = *it;

    return;
  }
}

feature::FeatureBuilder CreateFb(std::vector<m2::PointD> const & way, uint64_t id,
                                 uint32_t roadType, bool isRoundabout = true,
                                 bool isLeftHandTraffic = false)
{
  feature::FeatureBuilder fb;
  fb.SetOsmId(base::MakeOsmWay(id));
  fb.SetLinear();

  if (!isRoundabout)
  {
    UpdateFeatureGeometry(way, fb);
    return fb;
  }

  if (isLeftHandTraffic)
  {
    std::vector<m2::PointD> wayRev = way;
    std::reverse(wayRev.begin(), wayRev.end());

    UpdateFeatureGeometry(wayRev, fb);
    fb.AddPoint(wayRev[0]);
  }
  else
  {
    UpdateFeatureGeometry(way, fb);
    fb.AddPoint(way[0]);
  }

  static uint32_t const roundaboutType = classif().GetTypeByPath({"junction", "roundabout"});
  fb.AddType(roundaboutType);
  static uint32_t const defaultRoadType = classif().GetTypeByPath({"highway", "tertiary"});
  fb.AddType(roadType == 0 ? defaultRoadType : roadType);
  static uint32_t const onewayType = classif().GetTypeByPath({"hwtag", "oneway"});
  fb.AddType(onewayType);

  return fb;
}

feature::FeatureBuilder::PointSeq MiniRoundaboutTransformer::CreateSurrogateRoad(
    RoundaboutUnit const & roundaboutOnRoad, std::vector<m2::PointD> & roundaboutCircle,
    feature::FeatureBuilder::PointSeq & road,
    feature::FeatureBuilder::PointSeq::iterator & itPointUpd)
{
  feature::FeatureBuilder::PointSeq surrogateRoad(itPointUpd, road.end());
  auto itPointOnSurrogateRoad = surrogateRoad.begin();
  auto itPointSurrogateUpd = itPointOnSurrogateRoad;
  ++itPointOnSurrogateRoad;

  m2::PointD const nextPointOnSurrogateRoad = GetPointAtDistFromTarget(
      *itPointOnSurrogateRoad /* source */, roundaboutOnRoad.m_location /* target */,
      m_radiusMercator /* dist */);

  if (m2::AlmostEqualAbs(nextPointOnSurrogateRoad, *itPointOnSurrogateRoad, kMwmPointAccuracy))
    return {};

  AddPointToCircle(roundaboutCircle, nextPointOnSurrogateRoad);
  *itPointSurrogateUpd = nextPointOnSurrogateRoad;
  road.erase(itPointUpd + 1, road.end());

  return surrogateRoad;
}

bool MiniRoundaboutTransformer::AddRoundaboutToRoad(RoundaboutUnit const & roundaboutOnRoad,
                                                    std::vector<m2::PointD> & roundaboutCircle,
                                                    feature::FeatureBuilder::PointSeq & road,
                                                    std::vector<feature::FeatureBuilder> & newRoads)
{
  auto const roundaboutCenter = roundaboutOnRoad.m_location;
  auto itPointUpd = GetIterOnRoad(roundaboutCenter, road);

  CHECK(itPointUpd != road.end(), ());

  auto itPointNearRoundabout = itPointUpd;
  bool const isMiddlePoint = MoveIterAwayFromRoundabout(itPointNearRoundabout, road);
  m2::PointD const nextPointOnRoad =
      GetPointAtDistFromTarget(*itPointNearRoundabout /* source */, roundaboutCenter /* target */,
                               m_radiusMercator /* dist */);

  if (m2::AlmostEqualAbs(nextPointOnRoad, *itPointNearRoundabout, kMwmPointAccuracy))
    return false;

  if (isMiddlePoint)
  {
    auto const surrogateRoad =
        CreateSurrogateRoad(roundaboutOnRoad, roundaboutCircle, road, itPointUpd);
    if (surrogateRoad.size() < 2)
      return false;
    auto fbSurrogateRoad =
        CreateFb(surrogateRoad, roundaboutOnRoad.m_roadId, 0, false /* isRoundabout */);
    for (auto const & t : roundaboutOnRoad.m_roadTypes)
      fbSurrogateRoad.AddType(t);

    newRoads.push_back(std::move(fbSurrogateRoad));
  }

  AddPointToCircle(roundaboutCircle, nextPointOnRoad);
  *itPointUpd = nextPointOnRoad;
  return true;
}

std::unordered_map<base::GeoObjectId, size_t> GetFeaturesHashMap(
    std::vector<feature::FeatureBuilder> const & fbs)
{
  std::unordered_map<base::GeoObjectId, size_t> fbsIdToIndex;
  fbsIdToIndex.reserve(fbs.size());
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    if (routing::IsRoad(fbs[i].GetTypes()))
      fbsIdToIndex.insert(std::make_pair(fbs[i].GetMostGenericOsmId(), i));
  }
  return fbsIdToIndex;
}

void MiniRoundaboutTransformer::AddRoad(feature::FeatureBuilder && road)
{
  m_roads.emplace_back(std::move(road));
}

std::vector<feature::FeatureBuilder> MiniRoundaboutTransformer::ProcessRoundabouts()
{
  std::vector<feature::FeatureBuilder> fbsRoundabouts;
  fbsRoundabouts.reserve(m_roundabouts.size());
  // Some mini-roundabouts are mapped in the middle of the road. These roads should be split
  // in two parts on the opposite sides of the roundabout. New roads are saved in |fbsRoads|.
  std::vector<feature::FeatureBuilder> fbsRoads;
  fbsRoads.reserve(m_roundabouts.size());

  std::unordered_map<base::GeoObjectId, size_t> fbsIdToIndex = GetFeaturesHashMap(m_roads);

  for (auto const & rb : m_roundabouts)
  {
    m2::PointD const center = mercator::FromLatLon(rb.m_coord);
    std::vector<m2::PointD> circlePlain = PointToPolygon(center, m_radiusMercator);
    uint32_t roadType = 0;

    bool allRoadsInOneMwm = true;
    bool foundRoad = false;

    for (auto const & wayId : rb.m_ways)
    {
      base::GeoObjectId geoWayId = base::MakeOsmWay(wayId);
      // Way affiliated to the current mini_roundabout.
      auto pairIdIndex = fbsIdToIndex.find(geoWayId);
      if (pairIdIndex == fbsIdToIndex.end())
        continue;
      size_t const i = pairIdIndex->second;

      // Transform only mini_roundabouts on roads contained in single mwm
      if (m_affiliation->GetAffiliations(m_roads[i]).size() != 1)
      {
        allRoadsInOneMwm = false;
        break;
      }
      auto itRoad = m_roads.begin() + i;
      auto road = itRoad->GetOuterGeometry();

      if (GetIterOnRoad(center, road) == road.end())
      {
        bool foundSurrogateRoad = false;
        for (itRoad = fbsRoads.begin(); itRoad != fbsRoads.end(); ++itRoad)
        {
          if (itRoad->GetMostGenericOsmId() != geoWayId)
            continue;

          road = itRoad->GetOuterGeometry();
          if (GetIterOnRoad(center, road) == road.end())
            continue;

          foundSurrogateRoad = true;
          break;
        }

        if (!foundSurrogateRoad)
        {
          LOG(LERROR, ("Road not found for mini_roundabout", rb.m_coord));
          continue;
        }
      }

      RoundaboutUnit roundaboutOnRoad = {wayId, center, itRoad->GetTypes()};
      if (!AddRoundaboutToRoad(roundaboutOnRoad, circlePlain, road, fbsRoads))
        continue;

      UpdateFeatureGeometry(road, *itRoad);
      UpdateRoadType(itRoad->GetTypes(), roadType);
      foundRoad = true;
    }

    if (!allRoadsInOneMwm || !foundRoad)
      continue;

    fbsRoundabouts.push_back(
        CreateFb(circlePlain, rb.m_id, roadType, true /* isRoundabout */, m_leftHandTraffic));
  }

  LOG(LINFO, ("Transformed", fbsRoundabouts.size(), "mini_roundabouts to roundabouts.", "Added",
              fbsRoads.size(), "surrogate roads."));

  fbsRoundabouts.insert(fbsRoundabouts.end(), std::make_move_iterator(fbsRoads.begin()),
                        std::make_move_iterator(fbsRoads.end()));
  fbsRoundabouts.insert(fbsRoundabouts.end(), std::make_move_iterator(m_roads.begin()),
                        std::make_move_iterator(m_roads.end()));
  return fbsRoundabouts;
}

double DistanceOnPlain(m2::PointD const & a, m2::PointD const & b) { return a.Length(b); }

m2::PointD GetPointAtDistFromTarget(m2::PointD const & source, m2::PointD const & target,
                                    double dist)
{
  double const len = DistanceOnPlain(source, target);
  if (len < dist)
    return source;

  double const k = (len - dist) / dist;
  return (source + target * k) / (1.0 + k);
}

std::vector<m2::PointD> PointToPolygon(m2::PointD const & center, double radiusMercator,
                                       size_t verticesCount, double initAngleDeg)
{
  CHECK_GREATER(verticesCount, 2, ());
  CHECK_GREATER(radiusMercator, 0.0, ());

  std::vector<m2::PointD> vertices;
  vertices.reserve(verticesCount);

  double const kAngularPitch = 2 * math::pi / static_cast<double>(verticesCount);
  double angle = base::DegToRad(initAngleDeg);

  for (size_t i = 0; i < verticesCount; ++i)
  {
    vertices.emplace_back(center.x + radiusMercator * cos(angle),
                          center.y + radiusMercator * sin(angle));
    angle += kAngularPitch;
  }

  return vertices;
}

void AddPointToCircle(std::vector<m2::PointD> & circle, m2::PointD const & point)
{
  size_t iDist1 = 0;
  size_t iDist2 = 0;
  double dist1 = std::numeric_limits<double>::max();
  double dist2 = std::numeric_limits<double>::max();

  for (size_t i = 0; i < circle.size(); ++i)
  {
    double const dist = DistanceOnPlain(circle[i], point);
    if (dist < dist1)
    {
      dist2 = dist1;
      iDist2 = iDist1;

      dist1 = dist;
      iDist1 = i;
    }
    else if (dist < dist2)
    {
      dist2 = dist;
      iDist2 = i;
    }
  }

  if (iDist1 > iDist2)
    std::swap(iDist1, iDist2);

  if (iDist1 == 0 && iDist2 == circle.size() - 1)
    circle.push_back(point);
  else
    circle.insert(circle.begin() + iDist2, point);
}
}  // namespace generator
