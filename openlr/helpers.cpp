#include "openlr/helpers.hpp"

#include "openlr/road_info_getter.hpp"

#include "routing/features_road_graph.hpp"

#include "geometry/mercator.hpp"

#include <sstream>
#include <string>
#include <type_traits>

namespace
{
openlr::FunctionalRoadClass HighwayClassToFunctionalRoadClass(ftypes::HighwayClass const & hwClass)
{
  switch (hwClass)
  {
  case ftypes::HighwayClass::Trunk: return openlr::FunctionalRoadClass::FRC0;
  case ftypes::HighwayClass::Primary: return openlr::FunctionalRoadClass::FRC1;
  case ftypes::HighwayClass::Secondary: return openlr::FunctionalRoadClass::FRC2;
  case ftypes::HighwayClass::Tertiary: return openlr::FunctionalRoadClass::FRC3;
  case ftypes::HighwayClass::LivingStreet: return openlr::FunctionalRoadClass::FRC4;
  case ftypes::HighwayClass::Service: return openlr::FunctionalRoadClass::FRC5;
  default: return openlr::FunctionalRoadClass::FRC7;
  }
}
}  // namespace

namespace openlr
{
bool PointsAreClose(m2::PointD const & p1, m2::PointD const & p2)
{
  double const kMwmRoadCrossingRadiusMeters = routing::GetRoadCrossingRadiusMeters();
  return MercatorBounds::DistanceOnEarth(p1, p2) < kMwmRoadCrossingRadiusMeters;
}

double EdgeLength(Graph::Edge const & e)
{
  return MercatorBounds::DistanceOnEarth(e.GetStartPoint(), e.GetEndPoint());
}

bool EdgesAreAlmostEqual(Graph::Edge const & e1, Graph::Edge const & e2)
{
  // TODO(mgsergio): Do I need to check fields other than points?
  return PointsAreClose(e1.GetStartPoint(), e2.GetStartPoint()) &&
         PointsAreClose(e1.GetEndPoint(), e2.GetEndPoint());
}

std::string LogAs2GisPath(Graph::EdgeVector const & path)
{
  CHECK(!path.empty(), ("Paths should not be empty"));

  std::ostringstream ost;
  ost << "https://2gis.ru/moscow?queryState=";

  auto ll = MercatorBounds::ToLatLon(path.front().GetStartPoint());
  ost << "center%2F" << ll.lon << "%2C" << ll.lat << "%2F";
  ost << "zoom%2F" << 17 << "%2F";
  ost << "ruler%2Fpoints%2F";
  for (auto const & e : path)
  {
    ll = MercatorBounds::ToLatLon(e.GetStartPoint());
    ost << ll.lon << "%20" << ll.lat << "%2C";
  }
  ll = MercatorBounds::ToLatLon(path.back().GetEndPoint());
  ost << ll.lon << "%20" << ll.lat;

  return ost.str();
}

std::string LogAs2GisPath(Graph::Edge const & e) { return LogAs2GisPath(Graph::EdgeVector({e})); }

bool PassesRestriction(Graph::Edge const & e, FunctionalRoadClass restriction, FormOfWay fow,
                       int frcThreshold, RoadInfoGetter & infoGetter)
{
  if (e.IsFake() || restriction == FunctionalRoadClass::NotAValue)
    return true;

  auto const frc = HighwayClassToFunctionalRoadClass(infoGetter.Get(e.GetFeatureId()).m_hwClass);
  return static_cast<int>(frc) <= static_cast<int>(restriction) + frcThreshold;
}

bool ConformLfrcnp(Graph::Edge const & e, FunctionalRoadClass lowestFrcToNextPoint,
                   int frcThreshold, RoadInfoGetter & infoGetter)
{
  if (e.IsFake() || lowestFrcToNextPoint == FunctionalRoadClass::NotAValue)
    return true;

  auto const frc = HighwayClassToFunctionalRoadClass(infoGetter.Get(e.GetFeatureId()).m_hwClass);
  return static_cast<int>(frc) <= static_cast<int>(lowestFrcToNextPoint) + frcThreshold;
}
}  // namespace openlr
