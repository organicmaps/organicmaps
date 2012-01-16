#pragma once

#include "../../../../../platform/video_timer.hpp"

namespace android
{
  class VideoTimer : public ::VideoTimer
  {
  public:
    VideoTimer();
    virtual void start();
    virtual void stop();
    virtual void resume();
    virtual void pause();
  };
}
