#include "testing/testing.hpp"

#include "drape_frontend/user_event_stream.hpp"

#include "base/thread.hpp"

#include <cmath>
#include <cstring>
#include <functional>
#include <list>

using namespace std::placeholders;

#ifdef DEBUG

namespace
{
class UserEventStreamTest : df::UserEventStream::Listener
{
public:
  explicit UserEventStreamTest(bool filtrateTouches) : m_filtrate(filtrateTouches)
  {
    m_stream.SetTestBridge(std::bind(&UserEventStreamTest::TestBridge, this, _1));
  }

  void OnTap(m2::PointD const & pt, bool isLong) override {}
  void OnForceTap(m2::PointD const & pt) override {}
  void OnDoubleTap(m2::PointD const & pt) override {}
  void OnTwoFingersTap() override {}
  bool OnSingleTouchFiltrate(m2::PointD const & pt, df::TouchEvent::ETouchType type) override { return m_filtrate; }
  void OnDragStarted() override {}
  void OnDragEnded(m2::PointD const & /* distance */) override {}
  void OnRotated() override {}
  void OnScrolled(m2::PointD const & distance) override {}

  void OnScaleStarted() override {}
  void CorrectScalePoint(m2::PointD & pt) const override {}
  void CorrectScalePoint(m2::PointD & pt1, m2::PointD & pt2) const override {}
  void CorrectGlobalScalePoint(m2::PointD & pt) const override {}
  void OnScaleEnded() override {}
  void OnTouchMapAction(df::TouchEvent::ETouchType touchType, bool isMapTouch) override {}
  void OnAnimatedScaleEnded() override {}
  bool OnNewVisibleViewport(m2::RectD const & oldViewport, m2::RectD const & newViewport, bool needOffset,
                            m2::PointD & gOffset) override
  {
    return false;
  }

  void AddUserEvent(df::TouchEvent const & event) { m_stream.AddEvent(make_unique_dp<df::TouchEvent>(event)); }

  void SetRect(m2::RectD const & r)
  {
    m_stream.AddEvent(make_unique_dp<df::SetRectEvent>(r, false /* rotate */, -1, false /* isAnim */,
                                                       false /* useVisibleViewport */,
                                                       nullptr /* parallelAnimCreator */));
  }

  void AddResizeEvent(uint32_t w, uint32_t h) { m_stream.AddEvent(make_unique_dp<df::ResizeEvent>(w, h)); }

  void AddSetVisibleViewport(m2::RectD const & rect)
  {
    m_stream.AddEvent(make_unique_dp<df::SetVisibleViewportEvent>(rect));
  }

  void AddSetCenter(m2::PointD const & center, int zoom, bool trackVisibleViewport)
  {
    m_stream.AddEvent(make_unique_dp<df::SetCenterEvent>(center, zoom, false /* isAnim */, trackVisibleViewport,
                                                         nullptr /* parallelAnimCreator */));
  }

  ScreenBase const & GetScreen() const { return m_stream.GetCurrentScreen(); }

  void AddExpectation(char const * action) { m_expectation.push_back(action); }

  void RunTest()
  {
    bool dummy1, dummy2, dummy3;
    m_stream.ProcessEvents(dummy1, dummy2, dummy3);
    TEST_EQUAL(m_expectation.empty(), true, ());
  }

private:
  void TestBridge(char const * action)
  {
    TEST(!m_expectation.empty(), ());
    char const * a = m_expectation.front();
    TEST_EQUAL(strcmp(action, a), 0, ());
    m_expectation.pop_front();
  }

private:
  df::UserEventStream m_stream;
  std::list<char const *> m_expectation;
  bool m_filtrate;
};

int touchTimeStamp = 1;

df::TouchEvent MakeTouchEvent(m2::PointD const & pt1, m2::PointD const & pt2, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;
  df::Touch t1;
  t1.m_location = pt1;
  t1.m_id = 1;
  e.SetFirstTouch(t1);

  df::Touch t2;
  t2.m_location = pt2;
  t2.m_id = 2;
  e.SetSecondTouch(t2);

  e.SetTouchType(type);
  e.SetFirstMaskedPointer(0);
  e.SetSecondMaskedPointer(1);
  e.SetTimeStamp(touchTimeStamp++);

  return e;
}

df::TouchEvent MakeTouchEvent(m2::PointD const & pt, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;

  df::Touch t1;
  t1.m_location = pt;
  t1.m_id = 1;
  e.SetFirstTouch(t1);

  e.SetTouchType(type);
  e.SetFirstMaskedPointer(0);
  e.SetTimeStamp(touchTimeStamp++);

  return e;
}
}  // namespace

UNIT_TEST(SimpleTap)
{
  UserEventStreamTest test(false);
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::SHORT_TAP_DETECTED);

  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_UP));

  test.RunTest();
}

UNIT_TEST(SimpleLongTap)
{
  UserEventStreamTest test(false);
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_DOWN));
  test.RunTest();

  threads::Sleep(1100);
  test.AddExpectation(df::UserEventStream::LONG_TAP_DETECTED);
  test.RunTest();

  test.AddUserEvent(MakeTouchEvent(m2::PointD::Zero(), df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_TEST(SimpleDrag)
{
  size_t const moveEventCount = 5;
  UserEventStreamTest test(false);
  test.AddExpectation(df::UserEventStream::TRY_FILTER);
  test.AddExpectation(df::UserEventStream::BEGIN_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::CANCEL_TAP_DETECTOR);
  test.AddExpectation(df::UserEventStream::BEGIN_DRAG);
  for (size_t i = 0; i < moveEventCount - 2; ++i)
    test.AddExpectation(df::UserEventStream::DRAG);
  test.AddExpectation(df::UserEventStream::END_DRAG);

  m2::PointD pointer = m2::PointD::Zero();
  test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_DOWN));
  for (size_t i = 0; i < 5; ++i)
  {
    pointer += m2::PointD(100.0, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_TEST(SimpleScale)
{
  size_t const moveEventCount = 5;
  UserEventStreamTest test(false);
  test.SetRect(m2::RectD(-250.0, -250.0, 250.0, 250.0));

  test.AddExpectation(df::UserEventStream::TWO_FINGERS_TAP);
  test.AddExpectation(df::UserEventStream::BEGIN_SCALE);
  for (size_t i = 0; i < moveEventCount - 1; ++i)
    test.AddExpectation(df::UserEventStream::SCALE);
  test.AddExpectation(df::UserEventStream::END_SCALE);

  m2::PointD pointer1 = m2::PointD::Zero() + m2::PointD(10.0, 10.0);
  m2::PointD pointer2 = m2::PointD::Zero() - m2::PointD(10.0, 10.0);
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_DOWN));
  for (size_t i = 0; i < moveEventCount; ++i)
  {
    pointer1 += m2::PointD(20.0, 0.0);
    pointer2 -= m2::PointD(20.0, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_TEST(SetCenter_AlignsToVisibleViewportCenter)
{
  // With an asymmetric visible viewport (e.g. host UI panel covering bottom 30%),
  // SetCenter must place the geographic target at the visible-viewport center
  // regardless of trackVisibleViewport. The flag only controls whether the engine
  // continues to follow the viewport when it changes later.
  uint32_t const kW = 1000, kH = 1000;
  m2::RectD const kVisibleViewport(0.0, 0.0, static_cast<double>(kW), 700.0);
  m2::PointD const kTarget(0.0, 0.0);
  double const kEps = 1.0;
  m2::PointD const viewportCenter = kVisibleViewport.Center();

  for (bool trackVisibleViewport : {false, true})
  {
    UserEventStreamTest test(false);
    test.AddResizeEvent(kW, kH);
    test.AddSetVisibleViewport(kVisibleViewport);
    test.SetRect(m2::RectD(-100.0, -100.0, 100.0, 100.0));
    test.RunTest();

    test.AddSetCenter(kTarget, df::kDoNotChangeZoom, trackVisibleViewport);
    test.RunTest();

    m2::PointD const pixelPos = test.GetScreen().GtoP(kTarget);
    TEST(std::fabs(pixelPos.x - viewportCenter.x) < kEps,
         (pixelPos.x, "expected", viewportCenter.x, "trackVisibleViewport", trackVisibleViewport));
    TEST(std::fabs(pixelPos.y - viewportCenter.y) < kEps,
         (pixelPos.y, "expected", viewportCenter.y, "trackVisibleViewport", trackVisibleViewport));
  }
}

#endif
