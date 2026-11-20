#pragma once

#include "drape_frontend/user_event_stream.hpp"

#include <QList>
#include <QTouchEvent>

#include <vector>

namespace sailfish
{
// The core touch events of one Qt touch update, for its first two fingers. A finger lifted while another lands
// ends the old gesture before the new one starts.
std::vector<df::TouchEvent> ToDrapeTouchEvents(QList<QTouchEvent::TouchPoint> const & points);
// The fingers still down after the update, to end the gesture with when it is cancelled.
df::TouchEvent HeldTouches(QList<QTouchEvent::TouchPoint> const & points);
}  // namespace sailfish
