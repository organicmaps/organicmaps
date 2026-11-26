#pragma once

#include "drape_frontend/user_event_stream.hpp"

#include <QList>
#include <QTouchEvent>

#include <array>
#include <vector>

namespace sailfish
{
// The core touch events of one Qt touch update, for the first two fingers before and after it. A finger lifted while
// another lands ends the old gesture before the new one starts.
std::vector<df::TouchEvent> ToDrapeTouchEvents(QList<QTouchEvent::TouchPoint> const & points);
// The fingers still down after the update, to end the gesture with when it is cancelled.
std::array<df::Touch, 2> HeldTouches(QList<QTouchEvent::TouchPoint> const & points);
}  // namespace sailfish
