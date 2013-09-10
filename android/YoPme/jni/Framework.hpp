#pragma once

#include "../../../map/framework.hpp"
#include "../../../platform/video_timer.hpp"

#include "../../../std/shared_ptr.hpp"

namespace yopme
{
  class Framework
  {
  public:
    Framework(int width, int height);
    ~Framework();

    void ShowRect(double lat, double lon, double zoom);

  private:
    void InitRenderPolicy();
    void TeardownRenderPolicy();
    void RenderMap();

  private:
    ::Framework m_framework;
    int m_width;
    int m_height;
  };
}
