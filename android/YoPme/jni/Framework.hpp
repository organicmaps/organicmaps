#pragma once

#include "../../../map/framework.hpp"
#include "../../../platform/video_timer.hpp"

namespace yopme
{
  class Framework
  {
  public:
    Framework(int width, int height);
    ~Framework();

    bool ShowMyPosition(double lat, double lon, double zoom);
    bool ShowPoi(double lat, double lon, bool needMyLoc, double myLat, double myLoc, double zoom);

  private:
    void ShowRect(bool needApiPin, m2::PointD const & apiPinPoint,
                  bool needMyLoc, m2::PointD const & myLocPoint);
    void InitRenderPolicy(bool needApiPin, m2::PointD const & apiPinPoint,
                          bool needMyLoc, m2::PointD const & myLocPoint);

    void TeardownRenderPolicy();
    void RenderMap();

  private:
    ::Framework m_framework;
    EmptyVideoTimer m_timer;
    int m_width;
    int m_height;
  };
}
