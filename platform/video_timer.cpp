#include "video_timer.hpp"

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
