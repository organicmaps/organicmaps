#pragma once

#include "map/nogps/clock.hpp"
#include "map/nogps/engine.hpp"
#include "map/nogps/map_api.hpp"
#include "map/nogps/roads.hpp"
#include "map/nogps/scheduler.hpp"
#include "map/nogps/storage.hpp"

#include <functional>

class SessionRoute;

namespace nogps
{
/// The roads and the route of the core.
class RoutingMapApi : public MapApi
{
public:
  using ShowCarHeadingFn = std::function<void(double bearingDeg)>;
  using ShowOffRoadCarFn = std::function<void(std::optional<ms::LatLon> const & position)>;

  RoutingMapApi(SessionRoute & route, Clock const & clock, ShowCarHeadingFn && showCarHeading,
                ShowOffRoadCarFn && showOffRoadCar);

  // MapApi overrides:
  bool IsNavigating() const override;
  Roads & GetRoads(bool matchRoute) override { return matchRoute ? m_routeRoads : m_roads; }
  void RebuildRouteIfOffRoute(Fix const & fix, double offRouteDistanceM, std::optional<double> bearingDeg) override;
  std::optional<RoadPoint> ProjectToRoute(ms::LatLon const & position, double radiusM) override;
  std::optional<RoadWalker::Result> ShiftAlongRoute(ms::LatLon const & position, std::optional<double> bearingDeg,
                                                    double distanceM) override;
  std::optional<RoadPoint> SnapToMainRoad(ms::LatLon const & position, double radiusM) override;
  std::string GetDistanceLeft() const override;
  void ShowCarHeading(double bearingDeg) override { m_showCarHeading(bearingDeg); }
  void ShowOffRoadCar(std::optional<ms::LatLon> const & position) override { m_showOffRoadCar(position); }

private:
  class RoutingRoads : public Roads
  {
  public:
    RoutingRoads(SessionRoute & route, bool matchRoute) : m_route(route), m_matchRoute(matchRoute) {}

    std::optional<RoadPoint> Snap(ms::LatLon const & point, std::optional<double> bearingDeg, double radiusM) override;
    std::vector<ms::LatLon> FindCrossings(ms::LatLon const & center, double radiusM) override;
    bool IsRoundaboutNear(ms::LatLon const & center, double radiusM) override;

  private:
    SessionRoute & m_route;
    bool const m_matchRoute;
  };

  SessionRoute & m_route;
  Clock const & m_clock;
  ShowCarHeadingFn m_showCarHeading;
  ShowOffRoadCarFn m_showOffRoadCar;
  RoutingRoads m_roads;
  RoutingRoads m_routeRoads;
};

/// The navigation without GPS with everything it takes from the core.
class Service
{
public:
  Service(Delegate & delegate, SessionRoute & route, RoutingMapApi::ShowCarHeadingFn && showCarHeading,
          RoutingMapApi::ShowOffRoadCarFn && showOffRoadCar);

  Engine & GetEngine() { return m_engine; }
  Clock const & GetClock() const { return m_clock; }

  // The firmwares of the ESP32 sensor box among the resources of the application: the list of a release as
  // tools/make_release.py of the firmware writes it, and its files next to it.
  static constexpr char const * kFirmwareDir = "nogps-firmware/";
  static constexpr char const * kFirmwareList = "firmware.json";

private:
  SystemClock m_clock;
  PlatformScheduler m_scheduler;
  SettingsStorage m_storage;
  RoutingMapApi m_map;
  Engine m_engine;
};
}  // namespace nogps
