#include "Stubs.hpp"

#include "../../../std/bind.hpp"

namespace yopme
{

void RenderContext::makeCurrent()
{
}

graphics::RenderContext * RenderContext::createShared()
{
  return this;
}

void empty()
{
}

EmptyVideoTimer::EmptyVideoTimer()
    : VideoTimer(bind(&empty))
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

} // namespace yopme
