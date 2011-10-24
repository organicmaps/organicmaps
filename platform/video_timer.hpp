#pragma once

#include "../std/function.hpp"

/// Timer, synchronized to Vertical Sync
class VideoTimer
{
public:

  typedef function<void()> TFrameFn;

  enum EState
  {
    EStopped,
    EPaused,
    ERunning
  };

protected:

  TFrameFn m_frameFn;
  EState m_state;

public:
  VideoTimer(TFrameFn fn);
  virtual ~VideoTimer();

  TFrameFn frameFn() const;
  void setFrameFn(TFrameFn fn);

  EState state() const;

  virtual void resume() = 0;
  virtual void pause() = 0;

  virtual void start() = 0;
  virtual void stop() = 0;
};

extern "C" VideoTimer * CreateIOSVideoTimer(VideoTimer::TFrameFn frameFn);
extern "C" VideoTimer * CreateAppleVideoTimer(VideoTimer::TFrameFn frameFn);
extern "C" VideoTimer * CreateWin32VideoTimer(VideoTimer::TFrameFn frameFn);
