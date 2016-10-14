#pragma once

#include "routing/routing_settings.hpp"
#include "routing/turns.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/polyline2d.hpp"

#include "base/followed_polyline.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"
#include "std/string.hpp"


namespace location
{
  class GpsInfo;
  class RouteMatchingInfo;
}

namespace routing
{

class Route
{
public:
  using TTurns = vector<turns::TurnItem>;
  using TTimeItem = pair<uint32_t, double>;
  using TTimes = vector<TTimeItem>;
  using TStreetItem = pair<uint32_t, string>;
  using TStreets = vector<TStreetItem>;

  explicit Route(string const & router)
    : m_router(router), m_routingSettings(GetCarRoutingSettings()) {}

  template <class TIter>
  Route(string const & router, TIter beg, TIter end)
    : m_router(router), m_routingSettings(GetCarRoutingSettings()), m_poly(beg, end)
  {
    Update();
  }

  Route(string const & router, vector<m2::PointD> const & points, string const & name = string());

  void Swap(Route & rhs);

  template <class TIter> void SetGeometry(TIter beg, TIter end)
  {
    if (beg == end)
      FollowedPolyline().Swap(m_poly);
    else
      FollowedPolyline(beg, end).Swap(m_poly);
    Update();
  }

  inline void SetTurnInstructions(TTurns && v) { m_turns = move(v); }
  inline void SetSectionTimes(TTimes && v) { m_times = move(v); }
  inline void SetStreetNames(TStreets && v) { m_streets = move(v); }
  inline void SetAltitudes(feature::TAltitudes && v) { m_altitudes = move(v); }

  /// \brief Appends all |route| attributes except for altitude.
  void AppendRoute(Route const & route);

  uint32_t GetTotalTimeSec() const;
  uint32_t GetCurrentTimeToEndSec() const;

  FollowedPolyline const & GetFollowedPolyline() const { return m_poly; }

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly.GetPolyline(); }
  TTurns const & GetTurns() const { return m_turns; }
  feature::TAltitudes const & GetAltitudes() const { return m_altitudes; }
  vector<double> const & GetSegDistanceM() const { return m_poly.GetSegDistanceM(); }
  void GetTurnsDistances(vector<double> & distances) const;
  string const & GetName() const { return m_name; }
  bool IsValid() const { return (m_poly.GetPolyline().GetSize() > 1); }

  double GetTotalDistanceMeters() const;
  double GetCurrentDistanceFromBeginMeters() const;
  double GetCurrentDistanceToEndMeters() const;
  double GetMercatorDistanceFromBegin() const;

  /// \brief GetCurrentTurn returns information about the nearest turn.
  /// \param distanceToTurnMeters is a distance from current position to the nearest turn.
  /// \param turn is information about the nearest turn.
  bool GetCurrentTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;

  /// \brief Returns a name of a street where the user rides at this moment.
  void GetCurrentStreetName(string &) const;

  /// \brief Returns a name of a street next to idx point of the path. Function avoids short unnamed links.
  void GetStreetNameAfterIdx(uint32_t idx, string &) const;

  /// @return true if GetNextTurn() returns a valid result in parameters, false otherwise.
  /// \param distanceToTurnMeters is a distance from current position to the second turn.
  /// \param turn is information about the second turn.
  /// @return true if its parameters are filled with correct result.
  /// \note All parameters are filled while a GetNextTurn function call.
  bool GetNextTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;
  /// \brief Extract information about zero, one or two nearest turns depending on current position.
  /// @return true if its parameter is filled with correct result. (At least with one element.)
  bool GetNextTurns(vector<turns::TurnItemDist> & turns) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

  /// @return true  If position was updated successfully (projection within gps error radius).
  bool MoveIterator(location::GpsInfo const & info) const;

  void MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const;

  bool IsCurrentOnEnd() const;

  /// Add country name if we have no country filename to make route
  void AddAbsentCountry(string const & name);

  /// Get absent file list of a routing files for shortest path finding
  set<string> const & GetAbsentCountries() const { return m_absentCountries; }

  inline void SetRoutingSettings(RoutingSettings const & routingSettings)
  {
    m_routingSettings = routingSettings;
    Update();
  }

private:
  /// Call this fucnction when geometry have changed.
  void Update();
  double GetPolySegAngle(size_t ind) const;
  TTurns::const_iterator GetCurrentTurn() const;
  TStreets::const_iterator GetCurrentStreetNameIterAfter(FollowedPolyline::Iter iter) const;

private:
  friend string DebugPrint(Route const & r);

  string m_router;
  RoutingSettings m_routingSettings;
  string m_name;

  FollowedPolyline m_poly;
  FollowedPolyline m_simplifiedPoly;

  set<string> m_absentCountries;

  TTurns m_turns;
  TTimes m_times;
  TStreets m_streets;
  feature::TAltitudes m_altitudes;

  mutable double m_currentTime;
};

} // namespace routing
