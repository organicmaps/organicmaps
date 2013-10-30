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

    bool ShowMap(double  vpLat,    double vpLon,  double zoom,
                 bool hasPoi,      double poiLat, double poiLon,
                 bool hasLocation, double myLat,  double myLon);

    void OnMapFileUpdate();
    void OnKmlFileUpdate();

    bool AreLocationsFarEnough(double lat1, double lon1, double lat2, double lon2) const;

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
