#include "libs/map/online_camera_fetcher.hpp"

#include "routing/speed_camera.hpp"
#include "coding/url.hpp"

#include "platform/http_client.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <chrono>
#include <mutex>

namespace routing
{
struct OverpassNode
{
  double lat = 0.0;
  double lon = 0.0;
  std::map<std::string, std::string> tags;
};

struct OverpassResponse
{
  std::vector<OverpassNode> elements;
};
}  // namespace routing



namespace
{
double ParseSpeedLimit(std::string const & maxspeedTag)
{
  if (maxspeedTag.empty())
    return routing::SpeedCameraOnRoute::kNoSpeedInfo;

  try
  {
    size_t const firstDigit = maxspeedTag.find_first_of("0123456789");
    if (firstDigit == std::string::npos)
      return routing::SpeedCameraOnRoute::kNoSpeedInfo;

    size_t const lastDigit = maxspeedTag.find_last_of("0123456789");
    std::string const numStr = maxspeedTag.substr(firstDigit, lastDigit - firstDigit + 1);

    double val = std::stod(numStr);
    if (maxspeedTag.find("mph") != std::string::npos)
      val = measurement_utils::MiphToKmph(val);

    return val;
  }
  catch (...)
  {
    return routing::SpeedCameraOnRoute::kNoSpeedInfo;
  }
}
}  // namespace

namespace routing
{
namespace
{
// Query cache variables
double s_lastSouth = 0.0;
double s_lastWest = 0.0;
double s_lastNorth = 0.0;
double s_lastEast = 0.0;
std::vector<OnlineCamera> s_lastCameras;
std::chrono::steady_clock::time_point s_lastQueryTime;
std::mutex s_cacheMutex;
}  // namespace

// static
void OnlineCameraFetcher::FetchCamerasNearRect(m2::RectD const & rect, Callback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::Network, [rect, callback]()
  {
    ms::LatLon minLatLon = mercator::ToLatLon(rect.LeftBottom());
    ms::LatLon maxLatLon = mercator::ToLatLon(rect.RightTop());

    // Privacy protection: align to coarse 0.1 degree grid
    double const gridStep = 0.1;
    double const south = std::floor(std::min(minLatLon.m_lat, maxLatLon.m_lat) / gridStep) * gridStep;
    double const west = std::floor(std::min(minLatLon.m_lon, maxLatLon.m_lon) / gridStep) * gridStep;
    double const north = std::ceil(std::max(minLatLon.m_lat, maxLatLon.m_lat) / gridStep) * gridStep;
    double const east = std::ceil(std::max(minLatLon.m_lon, maxLatLon.m_lon) / gridStep) * gridStep;

    {
      std::lock_guard<std::mutex> lock(s_cacheMutex);
      auto const now = std::chrono::steady_clock::now();
      if (south == s_lastSouth && west == s_lastWest && north == s_lastNorth && east == s_lastEast &&
          std::chrono::duration_cast<std::chrono::seconds>(now - s_lastQueryTime).count() < 5)
      {
        std::vector<OnlineCamera> cachedCams = s_lastCameras;
        GetPlatform().RunTask(Platform::Thread::Gui, [callback, cachedCams = std::move(cachedCams)]()
        {
          callback(cachedCams);
        });
        return;
      }
    }

    if ((north - south) > 5.0 || (east - west) > 5.0)
    {
      LOG(LWARNING, ("Query bounding box is too large for online fetch, skipping:", south, west, north, east));
      return;
    }

    std::string const query = "[out:json][timeout:15];\n"
                              "(\n"
                              "  node[\"highway\"=\"speed_camera\"](" +
                              std::to_string(south) + "," + std::to_string(west) + "," +
                              std::to_string(north) + "," + std::to_string(east) + ");\n"
                              "  node[\"man_made\"=\"surveillance\"][\"surveillance:type\"=\"ALPR\"](" +
                              std::to_string(south) + "," + std::to_string(west) + "," +
                              std::to_string(north) + "," + std::to_string(east) + ");\n"
                              "  node[\"man_made\"=\"surveillance\"][\"surveillance:type\"=\"alpr\"](" +
                              std::to_string(south) + "," + std::to_string(west) + "," +
                              std::to_string(north) + "," + std::to_string(east) + ");\n"
                              "  node[\"man_made\"=\"surveillance\"][\"surveillance\"=\"traffic\"](" +
                              std::to_string(south) + "," + std::to_string(west) + "," +
                              std::to_string(north) + "," + std::to_string(east) + ");\n"
                              ");\n"
                              "out body;";

    platform::HttpClient request("https://overpass-api.de/api/interpreter");
    request.SetBodyData("data=" + url::UrlEncode(query), "application/x-www-form-urlencoded");
    request.SetRawHeader("User-Agent", "OrganicMaps/1.0 (https://organicmaps.app)");
    request.SetTimeout(15.0);

    if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
    {
      std::string const response = request.ServerResponse();
      routing::OverpassResponse responseObj;
      glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
      auto const error = glz::read<opts>(responseObj, response);
      if (!error)
      {
        std::vector<OnlineCamera> cameras;
        for (auto const & node : responseObj.elements)
        {
          OnlineCamera cam;
          cam.m_point = mercator::FromLatLon(node.lat, node.lon);

          auto const itSurveillance = node.tags.find("man_made");
          if (itSurveillance != node.tags.end() && itSurveillance->second == "surveillance")
          {
            cam.m_isAlpr = true;
            cam.m_speedLimitKmPH = SpeedCameraOnRoute::kAlprCameraSpeed;
          }
          else
          {
            cam.m_isAlpr = false;
            auto const itSpeed = node.tags.find("maxspeed");
            if (itSpeed != node.tags.end())
              cam.m_speedLimitKmPH = ParseSpeedLimit(itSpeed->second);
            else
              cam.m_speedLimitKmPH = SpeedCameraOnRoute::kNoSpeedInfo;
          }
          cameras.emplace_back(std::move(cam));
        }

        {
          std::lock_guard<std::mutex> lock(s_cacheMutex);
          s_lastSouth = south;
          s_lastWest = west;
          s_lastNorth = north;
          s_lastEast = east;
          s_lastCameras = cameras;
          s_lastQueryTime = std::chrono::steady_clock::now();
        }

        GetPlatform().RunTask(Platform::Thread::Gui, [callback, cameras = std::move(cameras)]()
        {
          callback(cameras);
        });
      }
      else
      {
        LOG(LWARNING, ("Failed to parse Overpass response:", glz::format_error(error, response)));
      }
    }
    else
    {
      LOG(LWARNING, ("Failed to fetch cameras from Overpass API. Code:", request.ErrorCode()));
    }
  });
}
}  // namespace routing
