#include "../std/vector.hpp"

namespace url_api
{

struct Point
{
  Point() : m_lat(0), m_lon(0) {}

  double m_lat;
  double m_lon;
  string m_name;
  string m_id;
};

struct Request
{
  vector<Point> m_points;
  double m_viewportLat, m_viewportLon, m_viewportZoomLevel;

  void Clear()
  {
    m_points.clear();
    m_viewportLat = m_viewportLon = m_viewportZoomLevel = 0;
  }
};

}  // namespace url_api
