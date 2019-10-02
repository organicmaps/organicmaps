#include "generator/mini_roundabout_transformer.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <unordered_map>

namespace
{
double constexpr kDefaultRadiusMeters = 5.0;
}

namespace generator
{
MiniRoundaboutTransformer::MiniRoundaboutTransformer(std::string const & intermediateFilePath)
  : m_radiusMercator(mercator::MetersToMercator(kDefaultRadiusMeters))
{
  ReadData(intermediateFilePath);
}

MiniRoundaboutTransformer::MiniRoundaboutTransformer(std::string const & intermediateFilePath,
                                                     double radiusMeters)
  : m_radiusMercator(mercator::MetersToMercator(radiusMeters))
{
  ReadData(intermediateFilePath);
}

void MiniRoundaboutTransformer::ReadData(std::string const & intermediateFilePath)
{
  m_roundabouts = ReadMiniRoundabouts(intermediateFilePath);
  LOG(LINFO, ("Loaded", m_roundabouts.size(), "mini_roundabouts from file", intermediateFilePath));
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
    if (itPrev == kHighwayTypes.end() || (itPrev > it && itPrev != kHighwayTypes.end()))
      roadType = *it;

    return;
  }
}

bool MiniRoundaboutTransformer::AddRoundaboutToRoad(m2::PointD const & center,
                                                    std::vector<m2::PointD> & roundabout,
                                                    feature::FeatureBuilder::PointSeq & road)
{
  auto itPointOnRoad =
      std::find_if(road.begin(), road.end(), [&center](m2::PointD const & pointOnRoad) {
        return base::AlmostEqualAbs(pointOnRoad, center, kMwmPointAccuracy);
      });

  CHECK(itPointOnRoad != road.end(),
        ("Could not find mini_roundabout on the road ", mercator::ToLatLon(center)));

  auto itPointUpd = itPointOnRoad;
  if (itPointOnRoad == road.begin())
  {
    ++itPointOnRoad;
  }
  else if (itPointOnRoad + 1 == road.end())
  {
    --itPointOnRoad;
  }
  else  // Roundabout is on the middle of the road so we need to insert 2 points
  {
    m2::PointD const leftPointOnRoad = TrimSegment(*(itPointOnRoad - 1), center, m_radiusMercator);
    if (AlmostEqualAbs(leftPointOnRoad, *(itPointOnRoad - 1), kMwmPointAccuracy))
      return false;

    AddPointToCircle(roundabout, leftPointOnRoad);
    itPointOnRoad = road.insert(itPointOnRoad, leftPointOnRoad);
    itPointUpd = itPointOnRoad + 1;
    itPointOnRoad += 2;
  }

  m2::PointD const nextPointOnRoad = TrimSegment(*itPointOnRoad, center, m_radiusMercator);
  if (AlmostEqualAbs(nextPointOnRoad, *itPointOnRoad, kMwmPointAccuracy))
    return false;

  AddPointToCircle(roundabout, nextPointOnRoad);
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

void UpdateRoadGeometry(feature::FeatureBuilder::PointSeq const & road,
                        feature::FeatureBuilder & fb)
{
  fb.ResetGeometry();
  for (auto const & p : road)
    fb.AddPoint(p);
}

feature::FeatureBuilder CreateRoundaboutFb(std::vector<m2::PointD> const & way, uint64_t wayId,
                                           uint32_t roadType)
{
  feature::FeatureBuilder fbRoundabout;
  fbRoundabout.SetLinear();

  for (auto const & point : way)
    fbRoundabout.AddPoint(point);

  fbRoundabout.AddPoint(way[0]);
  fbRoundabout.SetOsmId(base::MakeOsmWay(wayId));

  static uint32_t const roundaboutType = classif().GetTypeByPath({"junction", "roundabout"});
  fbRoundabout.AddType(roundaboutType);
  static uint32_t const defaultRoadType = classif().GetTypeByPath({"highway", "tertiary"});
  fbRoundabout.AddType(roadType == 0 ? defaultRoadType : roadType);

  return fbRoundabout;
}

void MiniRoundaboutTransformer::ProcessRoundabouts(
    feature::CountriesFilesIndexAffiliation const & affiliation,
    std::vector<feature::FeatureBuilder> & fbs)
{
  std::vector<feature::FeatureBuilder> fbsRoundabouts;
  fbsRoundabouts.reserve(m_roundabouts.size());

  std::unordered_map<base::GeoObjectId, size_t> fbsIdToIndex = GetFeaturesHashMap(fbs);

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
      if (affiliation.GetAffiliations(fbs[i]).size() != 1)
      {
        allRoadsInOneMwm = false;
        break;
      }

      auto road = fbs[i].GetOuterGeometry();

      if (!AddRoundaboutToRoad(center, circlePlain, road))
        continue;

      UpdateRoadGeometry(road, fbs[i]);
      UpdateRoadType(fbs[i].GetTypes(), roadType);
      foundRoad = true;
    }
    if (!allRoadsInOneMwm || !foundRoad)
      continue;

    fbsRoundabouts.push_back(CreateRoundaboutFb(circlePlain, rb.m_id, roadType));
  }

  // Adding new roundabouts to the features.
  fbs.insert(fbs.end(), std::make_move_iterator(fbsRoundabouts.begin()),
             std::make_move_iterator(fbsRoundabouts.end()));
  LOG(LINFO, ("Transformed", fbsRoundabouts.size(), "mini_roundabouts to roundabouts"));
}

double DistanceOnPlain(m2::PointD const & a, m2::PointD const & b) { return a.Length(b); }

m2::PointD TrimSegment(m2::PointD const & segPoint, m2::PointD const & target, double r)
{
  double const len = DistanceOnPlain(segPoint, target);
  if (len < r)
    return segPoint;

  double const k = (len - r) / r;
  return (segPoint + target * k) / (1.0 + k);
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
    if (AlmostEqualAbs(circle[i], point, kMwmPointAccuracy))
      return;

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

  CHECK(((iDist1 < iDist2) &&
         ((iDist1 == 0 && iDist2 == circle.size() - 1) || (iDist2 - iDist1 == 1))),
        ("Invalid conversion for point", mercator::ToLatLon(point)));

  if (iDist1 == 0 && iDist2 == circle.size() - 1)
    circle.push_back(point);
  else
    circle.insert(circle.begin() + iDist2, point);
}
}  // namespace generator
