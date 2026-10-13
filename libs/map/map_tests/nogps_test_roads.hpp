#pragma once

#include "map/nogps/geo.hpp"
#include "map/nogps/roads.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

// Roads made of meters east and north of a point, for the tests of the navigation without GPS.
namespace nogps_test
{
double constexpr kLat = 50.45;
double constexpr kLon = 30.52;
double constexpr kMPerDegLat = 111'320;

inline double MPerDegLon()
{
  return kMPerDegLat * std::cos(math::DegToRad(kLat));
}

inline ms::LatLon At(double eastM, double northM)
{
  return {kLat + northM / kMPerDegLat, kLon + eastM / MPerDegLon()};
}

inline double East(ms::LatLon const & p)
{
  return (p.m_lon - kLon) * MPerDegLon();
}

inline double North(ms::LatLon const & p)
{
  return (p.m_lat - kLat) * kMPerDegLat;
}

/// A grid of streets going east and north every 100 m, they cross at the multiples of 100 m. The part with
/// |x| or |y| more than 500 m has no roads.
class GridRoads : public nogps::Roads
{
public:
  static double constexpr kBlockM = 100;
  static double constexpr kSizeM = 500;

  std::optional<nogps::RoadPoint> Snap(ms::LatLon const & point, std::optional<double> bearingDeg,
                                       double radiusM) override
  {
    double const x = East(point);
    double const y = North(point);
    // The closest east street and the closest north street.
    double const eastStreetY = std::round(y / kBlockM) * kBlockM;
    double const northStreetX = std::round(x / kBlockM) * kBlockM;
    double const toEast = std::fabs(y - eastStreetY);
    double const toNorth = std::fabs(x - northStreetX);
    bool east = toEast <= toNorth;
    // A road going the car's way is preferred.
    if (bearingDeg)
    {
      bool const looksEast = std::fabs(std::sin(math::DegToRad(*bearingDeg))) > std::sqrt(0.5);
      if (looksEast && toEast <= radiusM)
        east = true;
      else if (!looksEast && toNorth <= radiusM)
        east = false;
    }
    if ((east ? toEast : toNorth) > radiusM || std::fabs(x) > kSizeM || std::fabs(y) > kSizeM)
      return {};
    if (east)
      return nogps::RoadPoint{{At(0, eastStreetY).m_lat, point.m_lon}, 90};
    return nogps::RoadPoint{{point.m_lat, At(northStreetX, 0).m_lon}, 0};
  }

  std::vector<ms::LatLon> FindCrossings(ms::LatLon const & center, double radiusM) override
  {
    double const x = East(center);
    double const y = North(center);
    std::vector<ms::LatLon> result;
    for (double cx = -kSizeM; cx <= kSizeM; cx += kBlockM)
      for (double cy = -kSizeM; cy <= kSizeM; cy += kBlockM)
        if (std::fabs(cx - x) <= radiusM && std::fabs(cy - y) <= radiusM)
          result.push_back(At(cx, cy));
    return result;
  }
};

/// Roads made of pieces {x1, y1, x2, y2} in meters east and north, snapped like the map does it: the closest
/// one going the car's way, not ending behind the car.
class PieceRoads : public nogps::Roads
{
public:
  using Piece = std::array<double, 4>;

  // |crossings| are {east, north} in meters.
  PieceRoads(std::vector<Piece> pieces, std::vector<std::pair<double, double>> crossings)
    : m_pieces(std::move(pieces))
    , m_crossings(std::move(crossings))
  {}

  std::optional<nogps::RoadPoint> Snap(ms::LatLon const & point, std::optional<double> bearingDeg,
                                       double radiusM) override
  {
    auto const roads = FindRoads(point, bearingDeg, radiusM);
    Found const * best = nullptr;
    for (auto const & road : roads)
      if (best == nullptr || road.m_cost < best->m_cost)
        best = &road;
    if (best == nullptr)
      return {};
    // The followed route is taken if it is as good as the best road or worse by 3 m, as the map does it.
    for (auto const & road : roads)
      if (road.m_isRoute && road.m_cost <= best->m_cost + 3)
        return road.m_road;
    return best->m_road;
  }

  std::vector<nogps::RoadPoint> SnapAll(ms::LatLon const & point, double bearingDeg, double radiusM) override
  {
    std::vector<nogps::RoadPoint> result;
    for (auto const & road : FindRoads(point, bearingDeg, radiusM))
      if (!road.m_wrongWay)
        result.push_back(road.m_road);
    return result;
  }

  /// The followed route goes along the piece.
  void SetRoutePiece(size_t index) { m_routePiece = index; }
  /// The piece is a one-way road from its first point to the second.
  void SetOneWay(size_t index) { m_oneWayPiece = index; }

  std::vector<ms::LatLon> FindCrossings(ms::LatLon const &, double) override
  {
    std::vector<ms::LatLon> result;
    for (auto const & [east, north] : m_crossings)
      result.push_back(At(east, north));
    return result;
  }

  /// A roundabout: a circle of the radius around the point. It has no pieces, a car is not snapped to it.
  void AddRoundabout(double eastM, double northM, double radiusM) { m_roundabouts.push_back({eastM, northM, radiusM}); }

  bool IsRoundaboutNear(ms::LatLon const & center, double radiusM) override
  {
    for (auto const & r : m_roundabouts)
      if (std::fabs(std::hypot(East(center) - r[0], North(center) - r[1]) - r[2]) <= radiusM)
        return true;
    return false;
  }

private:
  struct Found
  {
    nogps::RoadPoint m_road;
    double m_cost = 0;
    bool m_isRoute = false;
    bool m_wrongWay = false;
  };

  // The closest place of every piece the car may drive along.
  std::vector<Found> FindRoads(ms::LatLon const & point, std::optional<double> bearingDeg, double radiusM) const
  {
    double const x = East(point);
    double const y = North(point);
    std::vector<Found> result;
    for (size_t i = 0; i < m_pieces.size(); ++i)
    {
      auto const & p = m_pieces[i];
      double const dx = p[2] - p[0];
      double const dy = p[3] - p[1];
      double const projection = ((x - p[0]) * dx + (y - p[1]) * dy) / (dx * dx + dy * dy);
      double const t = std::clamp(projection, 0.0, 1.0);
      double const px = p[0] + t * dx;
      double const py = p[1] + t * dy;
      double const distance = std::hypot(px - x, py - y);
      double const roadBearing = math::RadToDeg(std::atan2(dx, dy));
      double diff = std::fabs(nogps::AngleDiff(roadBearing, bearingDeg ? *bearingDeg : roadBearing));
      diff = std::min(diff, 180 - diff);
      double const along = !bearingDeg ? 0
                                       : (px - x) * std::sin(math::DegToRad(*bearingDeg)) +
                                             (py - y) * std::cos(math::DegToRad(*bearingDeg));
      if (distance > radiusM || diff > 45 || (projection != t && along < -0.5))
        continue;
      // A one-way road going against the car is where the car is only if there is nothing else, as on the map.
      bool const wrongWay = bearingDeg && m_oneWayPiece && *m_oneWayPiece == i &&
                            std::fabs(nogps::AngleDiff(roadBearing, *bearingDeg)) > 90;
      result.push_back({{At(px, py), nogps::Normalize(roadBearing)},
                        distance + diff * 0.2 + (wrongWay ? 20 : 0),
                        m_routePiece && *m_routePiece == i,
                        wrongWay});
    }
    return result;
  }

  std::vector<Piece> m_pieces;
  std::optional<size_t> m_routePiece;
  std::optional<size_t> m_oneWayPiece;
  std::vector<std::pair<double, double>> m_crossings;
  std::vector<std::array<double, 3>> m_roundabouts;
};
}  // namespace nogps_test
