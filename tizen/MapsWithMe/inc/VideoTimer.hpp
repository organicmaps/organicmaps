#pragma once

#include <FBase.h>
#include "../../../map/framework.hpp"

namespace tizen
{

class VideoTimer1
  : public ::VideoTimer
  , public Tizen::Base::Runtime::ITimerEventListener
{
public:
  VideoTimer1(TFrameFn fn);
  virtual ~VideoTimer1();

  virtual void resume();
  virtual void pause();

  virtual void start();
  virtual void stop();

  void OnTimerExpired(Tizen::Base::Runtime::Timer & timer);
private:
  Tizen::Base::Runtime::Timer m_timer;
};

}
