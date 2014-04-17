#include "VideoTimer.hpp"

namespace tizen
{

VideoTimer1::VideoTimer1(TFrameFn frameFn)
: ::VideoTimer(frameFn)
{
  m_timer.Construct(*this);
}

VideoTimer1::~VideoTimer1()
{

}

void VideoTimer1::start()
{
  resume();
}

void VideoTimer1::pause()
{
  m_timer.Cancel();
  m_state = EPaused;
}

void VideoTimer1::resume()
{
  m_timer.StartAsRepeatable(1000/60);
  m_state = ERunning;
}

void VideoTimer1::stop()
{
  pause();
  m_state = EStopped;
}

void VideoTimer1::OnTimerExpired(Tizen::Base::Runtime::Timer & timer)
{
  m_frameFn();
}

}
