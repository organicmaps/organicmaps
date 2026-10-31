#include "touch_input.hpp"

#include <algorithm>

namespace gtk
{
bool TouchInput::TouchBegin(void const * sequence, m2::PointF const & pos)
{
  if (m_mousePressed || m_touches.size() >= 2 || Find(sequence) != m_touches.end())
    return false;
  m_touches.push_back({sequence, m_nextId++, pos});
  Send(df::TouchEvent::TOUCH_DOWN, static_cast<uint8_t>(m_touches.size() - 1));
  return true;
}

bool TouchInput::TouchUpdate(void const * sequence, m2::PointF const & pos)
{
  auto const it = Find(sequence);
  if (it == m_touches.end())
    return false;
  it->m_pos = pos;
  Send(df::TouchEvent::TOUCH_MOVE, m_touches.size() == 1 ? 0 : df::TouchEvent::INVALID_MASKED_POINTER);
  return true;
}

bool TouchInput::TouchEnd(void const * sequence, m2::PointF const & pos)
{
  auto const it = Find(sequence);
  if (it == m_touches.end())
    return false;
  it->m_pos = pos;
  auto const index = static_cast<uint8_t>(it - m_touches.begin());
  Send(df::TouchEvent::TOUCH_UP, index);
  m_touches.erase(it);
  return true;
}

bool TouchInput::TouchCancel(void const * sequence)
{
  if (Find(sequence) == m_touches.end())
    return false;
  // The engine tracks the gesture as a whole.
  Cancel();
  return true;
}

bool TouchInput::MouseDown(m2::PointF const & pos)
{
  if (!m_touches.empty())
    return false;
  m_mousePressed = true;
  m_touches.push_back({nullptr, m_nextId++, pos});
  Send(df::TouchEvent::TOUCH_DOWN, 0);
  return true;
}

bool TouchInput::MouseMove(m2::PointF const & pos)
{
  if (!m_mousePressed)
    return false;
  m_touches[0].m_pos = pos;
  Send(df::TouchEvent::TOUCH_MOVE, 0);
  return true;
}

bool TouchInput::MouseUp(m2::PointF const & pos)
{
  if (!m_mousePressed)
    return false;
  m_touches[0].m_pos = pos;
  Send(df::TouchEvent::TOUCH_UP, 0);
  m_touches.clear();
  m_mousePressed = false;
  return true;
}

void TouchInput::Cancel()
{
  if (!m_touches.empty())
    Send(df::TouchEvent::TOUCH_CANCEL, 0);
  m_touches.clear();
  m_mousePressed = false;
}

std::vector<TouchInput::ActiveTouch>::iterator TouchInput::Find(void const * sequence)
{
  if (m_mousePressed)
    return m_touches.end();
  return std::find_if(m_touches.begin(), m_touches.end(),
                      [sequence](ActiveTouch const & t) { return t.m_sequence == sequence; });
}

void TouchInput::Send(df::TouchEvent::ETouchType type, uint8_t maskedIndex)
{
  df::TouchEvent event;
  event.SetTouchType(type);
  df::Touch t1;
  t1.m_id = m_touches[0].m_id;
  t1.m_location = m_touches[0].m_pos;
  event.SetFirstTouch(t1);
  if (m_touches.size() > 1)
  {
    df::Touch t2;
    t2.m_id = m_touches[1].m_id;
    t2.m_location = m_touches[1].m_pos;
    event.SetSecondTouch(t2);
  }
  event.SetFirstMaskedPointer(maskedIndex);
  m_sink(event);
}
}  // namespace gtk
