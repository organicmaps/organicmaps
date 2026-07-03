#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include <functional>
#include <string>
#include <vector>

namespace routing
{
struct OnlineCamera
{
  m2::PointD m_point;
  bool m_isAlpr = false;
  double m_speedLimitKmPH = 0.0;
};

class OnlineCameraFetcher
{
public:
  using Callback = std::function<void(std::vector<OnlineCamera> const &)>;

  static void FetchCamerasNearRect(m2::RectD const & rect, Callback const & callback);
};
}  // namespace routing
