#pragma once

#include "../std/function.hpp"

/// Timer, synchronized to Vertical Sync
class VideoTimer
{
public:

  typedef function<void()> TFrameFn;

protected:

  TFrameFn m_frameFn;

public:
  VideoTimer(TFrameFn fn);
  virtual ~VideoTimer() {}

  virtual void start() = 0;
  virtual void stop() = 0;
};

extern "C" VideoTimer * CreateIOSVideoTimer(VideoTimer::TFrameFn frameFn);
extern "C" VideoTimer * CreateAppleVideoTimer(VideoTimer::TFrameFn frameFn);
extern "C" VideoTimer * CreateWin32VideoTimer(VideoTimer::TFrameFn frameFn);
