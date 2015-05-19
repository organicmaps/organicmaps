#include "testing/testing.hpp"

#include "drape_frontend/user_event_stream.hpp"

#include "base/thread.hpp"

#include "std/bind.hpp"
#include "std/list.hpp"

namespace
{

class UserEventStreamTest
{
public:
  UserEventStreamTest(bool filtrateTouches)
    : m_stream([](m2::PointD const &) { return true; })
  {
    auto const tapDetectedFn = [](m2::PointD const &, bool isLong) {};
    auto const filtrateFn = [&filtrateTouches](m2::PointD const &, df::TouchEvent::ETouchType)
    {
      return filtrateTouches;
    };

    m_stream.SetTapListener(tapDetectedFn, filtrateFn);
    m_stream.SetTestBridge(bind(&UserEventStreamTest::TestBridge, this, _1));
  }

  void AddUserEvent(df::TouchEvent const & event)
  {
    m_stream.AddEvent(event);
  }

  void SetRect(m2::RectD const & r)
  {
    m_stream.AddEvent(df::SetRectEvent(r, false, -1, false /* isAnim */));
  }

  void AddExpectation(char const * action)
  {
    m_expectation.push_back(action);
  }

  void RunTest()
  {
    bool dummy1, dummy2;
    m_stream.ProcessEvents(dummy1, dummy2);
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
  list<char const *> m_expectation;
};

df::TouchEvent MakeTouchEvent(m2::PointD const & pt1, m2::PointD const & pt2, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;
  e.m_touches[0].m_location = pt1;
  e.m_touches[0].m_id = 1;
  e.m_touches[1].m_location = pt2;
  e.m_touches[1].m_id = 2;
  e.m_type = type;

  return e;
}

df::TouchEvent MakeTouchEvent(m2::PointD const & pt, df::TouchEvent::ETouchType type)
{
  df::TouchEvent e;
  e.m_touches[0].m_location = pt;
  e.m_touches[0].m_id = 1;
  e.m_type = type;

  return e;
}

} // namespace

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
    pointer += m2::PointD(0.1, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}

UNIT_TEST(SimpleScale)
{
  size_t const moveEventCount = 5;
  UserEventStreamTest test(false);
  test.SetRect(m2::RectD(-10, -10, 10, 10));

  test.AddExpectation(df::UserEventStream::BEGIN_SCALE);
  for (size_t i = 0; i < moveEventCount; ++i)
    test.AddExpectation(df::UserEventStream::SCALE);
  test.AddExpectation(df::UserEventStream::END_SCALE);

  m2::PointD pointer1 = m2::PointD::Zero() + m2::PointD(0.1, 0.1);
  m2::PointD pointer2 = m2::PointD::Zero() - m2::PointD(0.1, 0.1);
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_DOWN));
  for (size_t i = 0; i < 5; ++i)
  {
    pointer1 += m2::PointD(0.1, 0.0);
    pointer2 -= m2::PointD(0.1, 0.0);
    test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_MOVE));
  }
  test.AddUserEvent(MakeTouchEvent(pointer1, pointer2, df::TouchEvent::TOUCH_UP));
  test.RunTest();
}
