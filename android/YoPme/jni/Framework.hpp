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

    void ConfigureNavigator(double lat, double lon, double zoom);
    void RenderMap();

  private:
    ::Framework m_framework;
    shared_ptr<VideoTimer> m_timer;
  };
}
