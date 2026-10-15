#pragma once

#include "drape_frontend/user_event_stream.hpp"

#include "geometry/point2d.hpp"

#include <functional>
#include <vector>

namespace gtk
{
// Turns touchscreen and primary mouse button events into the engine's touch events, which
// recognizes taps, drags, pinch and rotation itself. Up to two fingers are passed, like the Android
// client does; the mouse emulates one finger. Only one input kind is tracked at a time: touches
// are ignored while the mouse button is down and vice versa.
// Every method returns true if the event was used.
class TouchInput
{
public:
  using Sink = std::function<void(df::TouchEvent const &)>;

  explicit TouchInput(Sink sink) : m_sink(std::move(sink)) {}

  // |sequence| identifies a finger.
  bool TouchBegin(void const * sequence, m2::PointF const & pos);
  bool TouchUpdate(void const * sequence, m2::PointF const & pos);
  bool TouchEnd(void const * sequence, m2::PointF const & pos);
  bool TouchCancel(void const * sequence);

  bool MouseDown(m2::PointF const & pos);
  bool MouseMove(m2::PointF const & pos);
  bool MouseUp(m2::PointF const & pos);

  // Cancels the gesture in progress, if any.
  void Cancel();

private:
  struct ActiveTouch
  {
    void const * m_sequence = nullptr;
    int64_t m_id = -1;
    m2::PointF m_pos;
  };

  std::vector<ActiveTouch>::iterator Find(void const * sequence);
  void Send(df::TouchEvent::ETouchType type, uint8_t maskedIndex);

  Sink m_sink;
  std::vector<ActiveTouch> m_touches;
  int64_t m_nextId = 0;
  bool m_mousePressed = false;
};
}  // namespace gtk
