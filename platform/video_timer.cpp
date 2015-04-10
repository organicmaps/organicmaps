#include "platform/video_timer.hpp"

#include "std/bind.hpp"

VideoTimer::VideoTimer(TFrameFn fn) : m_frameFn(fn), m_state(EStopped)
{}

VideoTimer::EState VideoTimer::state() const
{
  return m_state;
}

VideoTimer::~VideoTimer()
{}

VideoTimer::TFrameFn VideoTimer::frameFn() const
{
  return m_frameFn;
}

void VideoTimer::setFrameFn(TFrameFn fn)
{
  m_frameFn = fn;
}

namespace
{
  void empty() {}
}

EmptyVideoTimer::EmptyVideoTimer()
    : base_t(bind(&empty))
{
}

EmptyVideoTimer::~EmptyVideoTimer()
{
  stop();
}

void EmptyVideoTimer::start()
{
  if (m_state == EStopped)
    m_state = ERunning;
}

void EmptyVideoTimer::resume()
{
  if (m_state == EPaused)
  {
    m_state = EStopped;
    start();
  }
}

void EmptyVideoTimer::pause()
{
  stop();
  m_state = EPaused;
}

void EmptyVideoTimer::stop()
{
  if (m_state == ERunning)
    m_state = EStopped;
}

void EmptyVideoTimer::perform()
{
}
