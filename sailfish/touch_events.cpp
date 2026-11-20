#include "sailfish/touch_events.hpp"

namespace sailfish
{
namespace
{
// The fingers that aren't in the skipped state, with the one in the changed state as the masked pointer, as in an
// Android MotionEvent.
df::TouchEvent MakeEvent(QList<QTouchEvent::TouchPoint> const & points, df::TouchEvent::ETouchType type,
                         Qt::TouchPointState skipped, Qt::TouchPointState changed)
{
  df::TouchEvent event;
  uint8_t maskedPointer = df::TouchEvent::INVALID_MASKED_POINTER;
  uint8_t count = 0;
  for (int i = 0; i < points.size() && count < 2; ++i)
  {
    if (points[i].state() == skipped)
      continue;
    df::Touch touch;
    touch.m_id = points[i].id();
    touch.m_location = m2::PointD(points[i].pos().x(), points[i].pos().y());
    if (count == 0)
      event.SetFirstTouch(touch);
    else
      event.SetSecondTouch(touch);
    if (points[i].state() == changed)
      maskedPointer = count;
    ++count;
  }
  event.SetTouchType(type);
  event.SetFirstMaskedPointer(maskedPointer);
  return event;
}
}  // namespace

std::vector<df::TouchEvent> ToDrapeTouchEvents(QList<QTouchEvent::TouchPoint> const & points)
{
  std::vector<df::TouchEvent> events;
  for (auto const & event :
       {MakeEvent(points, df::TouchEvent::TOUCH_UP, Qt::TouchPointPressed, Qt::TouchPointReleased),
        MakeEvent(points, df::TouchEvent::TOUCH_DOWN, Qt::TouchPointReleased, Qt::TouchPointPressed)})
  {
    if (event.GetFirstMaskedPointer() != df::TouchEvent::INVALID_MASKED_POINTER)
      events.push_back(event);
  }
  if (events.empty())
    events.push_back(MakeEvent(points, df::TouchEvent::TOUCH_MOVE, Qt::TouchPointState{}, Qt::TouchPointState{}));
  return events;
}

std::array<df::Touch, 2> HeldTouches(QList<QTouchEvent::TouchPoint> const & points)
{
  return MakeEvent(points, df::TouchEvent::TOUCH_NONE, Qt::TouchPointReleased, Qt::TouchPointState{}).GetTouches();
}
}  // namespace sailfish
