// Checks the Qt touch conversion with the core gesture detection. Built and run by the Sailfish OS CI job.

#include "sailfish/touch_events.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/visual_params.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <chrono>
#include <thread>

namespace
{
class Listener : public df::UserEventStream::Listener
{
public:
  void OnTap(m2::PointD const &, bool isLong) override { m_taps += isLong ? 0 : 1; }
  void OnForceTap(m2::PointD const &) override {}
  void OnDoubleTap(m2::PointD const &) override {}
  void OnTwoFingersTap() override {}
  bool OnSingleTouchFiltrate(m2::PointD const &, df::TouchEvent::ETouchType) override { return false; }
  void OnDragStarted() override { ++m_dragsStarted; }
  void OnDragEnded(m2::PointD const &) override {}
  void OnRotated() override {}
  void OnScrolled(m2::PointD const &) override {}
  void OnScaleStarted() override {}
  void CorrectScalePoint(m2::PointD &) const override {}
  void CorrectScalePoint(m2::PointD &, m2::PointD &) const override {}
  void CorrectGlobalScalePoint(m2::PointD &) const override {}
  void OnScaleEnded() override {}
  void OnTouchMapAction(df::TouchEvent::ETouchType, bool) override {}
  void OnScaleRequested() override {}
  bool OnNewVisibleViewport(m2::RectD const &, m2::RectD const &, bool, m2::PointD &) override { return false; }

  int m_dragsStarted = 0;
  int m_taps = 0;
};

QTouchEvent::TouchPoint Point(int id, Qt::TouchPointState state, double x, double y)
{
  QTouchEvent::TouchPoint point(id);
  point.setState(state);
  point.setPos(QPointF(x, y));
  return point;
}
}  // namespace

int main()
{
  df::VisualParams::Init(1.0 /* visualScale */, 1024 /* tileSize */);
  Listener listener;
  df::AnimationSystem animations;
  df::UserEventStream stream(animations);
  stream.SetListener(make_ref(&listener));
  auto const process = [&stream]
  {
    bool modelViewChanged, viewportChanged, activeFrame;
    stream.ProcessEvents(modelViewChanged, viewportChanged, activeFrame, df::AnimationClock::now());
  };
  auto const send = [&stream, &process](QList<QTouchEvent::TouchPoint> const & points)
  {
    for (auto const & event : sailfish::ToDrapeTouchEvents(points))
      stream.AddEvent(make_unique_dp<df::TouchEvent>(event));
    process();
  };

  // Finger 1 is lifted as finger 2 lands, in one update and listed in this order.
  send({Point(1, Qt::TouchPointPressed, 100, 100)});
  send({Point(1, Qt::TouchPointReleased, 100, 100), Point(2, Qt::TouchPointPressed, 300, 300)});
  send({Point(2, Qt::TouchPointReleased, 300, 300)});

  // A later pan with another finger, after the double tap time.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  send({Point(3, Qt::TouchPointPressed, 100, 500)});
  for (int i = 1; i <= 5; ++i)
    send({Point(3, Qt::TouchPointMoved, 100 + 100 * i, 500)});
  CHECK_EQUAL(listener.m_dragsStarted, 1, ("The next pan didn't start"));
  send({Point(3, Qt::TouchPointReleased, 600, 500)});

  // Fingers 4 and 5 are lifted as finger 6 lands, in one update, and finger 6 then taps.
  std::this_thread::sleep_for(std::chrono::seconds(1));
  send({Point(4, Qt::TouchPointPressed, 100, 100)});
  send({Point(4, Qt::TouchPointStationary, 100, 100), Point(5, Qt::TouchPointPressed, 300, 300)});
  send({Point(4, Qt::TouchPointReleased, 100, 100), Point(5, Qt::TouchPointReleased, 300, 300),
        Point(6, Qt::TouchPointPressed, 500, 500)});
  send({Point(6, Qt::TouchPointReleased, 500, 500)});
  std::this_thread::sleep_for(std::chrono::seconds(1));
  process();
  CHECK_EQUAL(listener.m_taps, 1, ("The tap of the new finger was missed"));

  LOG(LINFO, ("Touch events OK"));
  return 0;
}
