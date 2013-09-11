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

    bool ShowRect(double lat, double lon, double zoom, bool needApiMark);

  private:
    void InitRenderPolicy();
    void TeardownRenderPolicy();
    void RenderMap(const m2::PointD & markPoint, const string & symbolName);

  private:
    ::Framework m_framework;
    EmptyVideoTimer m_timer;
    int m_width;
    int m_height;
  };
}
