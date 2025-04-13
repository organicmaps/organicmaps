#pragma once

#include "routing/checkpoints.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/vehicle_mask.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"

#include "geometry/latlon.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace routing_quality
{
namespace api
{
enum class ResultCode
{
  ResponseOK,
  Error
};

enum class VehicleType
{
  Car
};

struct Params
{
  static void Dump(Params const & route, FileWriter & writer);
  static Params Load(ReaderSource<FileReader> & src);

  VehicleType m_type = VehicleType::Car;
  routing::Checkpoints m_waypoints;
};

struct Route
{
  static void Dump(Route const & route, FileWriter & writer);
  static Route Load(ReaderSource<FileReader> & src);

  std::vector<ms::LatLon> const & GetWaypoints() const { return m_waypoints; }
  double GetETA() const { return m_eta; }

  double m_eta = 0.0;
  double m_distance = 0.0;
  std::vector<ms::LatLon> m_waypoints;
};

struct Response
{
  static std::string const kDumpExtension;

  static void Dump(std::string const & filepath, Response const & response);
  static Response Load(std::string const & filepath);

  routing::VehicleType GetVehicleType() const;
  m2::PointD const & GetStartPoint() const { return m_params.m_waypoints.GetPointFrom(); }
  m2::PointD const & GetFinishPoint() const { return m_params.m_waypoints.GetPointTo(); }
  bool IsCodeOK() const { return m_code == api::ResultCode::ResponseOK; }
  std::vector<Route> const & GetRoutes() const { return m_routes; }

  ResultCode m_code = ResultCode::Error;
  Params m_params;
  std::vector<Route> m_routes;
};

class RoutingApiInterface
{
public:
  virtual ~RoutingApiInterface() = default;
  virtual Response CalculateRoute(Params const & params, int32_t startTimeZoneUTC) const = 0;
};

class RoutingApi : public RoutingApiInterface
{
public:
  RoutingApi(std::string name, std::string token, uint32_t maxRPS = 1);

  // RoutingApiInterface overrides:
  // @{
  Response CalculateRoute(Params const & params, int32_t startTimeZoneUTC) const override;
  // @}

  uint32_t GetMaxRPS() const;

  std::string const & GetApiName() const;
  std::string const & GetAccessToken() const;

private:
  std::string m_apiName;
  std::string m_accessToken;

  // Maximum requests per second provided with api
  uint32_t m_maxRPS;
};
}  // namespace api
}  // namespace routing_quality
