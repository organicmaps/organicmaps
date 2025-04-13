#pragma once

#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_engine.hpp"

#include "geometry/latlon.hpp"
#include "geometry/oblate_spheroid.hpp"
#include "geometry/point2d.hpp"

#include <array>
#include <vector>

namespace qt
{
class Ruler
{
public:
  bool IsActive();
  void SetActive(bool status);

  void AddPoint(m2::PointD const & point);

  void DrawLine(df::DrapeApi & drapeApi);
  void EraseLine(df::DrapeApi & drapeApi);

private:
  bool IsValidPolyline();
  void SetDistance();
  void SetId();

  std::string m_id;
  std::vector<m2::PointD> m_polyline;
  std::array<ms::LatLon, 2> m_pointsPair;
  bool m_isActive = false;
  double m_sumDistanceM = 0.0;
};
}  // namespace qt
