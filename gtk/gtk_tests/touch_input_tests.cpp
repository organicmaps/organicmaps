#include "testing/testing.hpp"

#include "gtk/touch_input.hpp"

#include <vector>

namespace touch_input_tests
{
using df::TouchEvent;

class Recorder
{
public:
  gtk::TouchInput m_input{[this](TouchEvent const & e) { m_events.push_back(e); }};
  std::vector<TouchEvent> m_events;

  TouchEvent const & Last() const
  {
    CHECK(!m_events.empty(), ());
    return m_events.back();
  }
};

int const kFinger1 = 1;
int const kFinger2 = 2;
m2::PointF const kA(10, 20);
m2::PointF const kB(30, 40);
m2::PointF const kC(50, 60);

UNIT_TEST(TouchInput_TwoFingers)
{
  Recorder r;
  TEST(r.m_input.TouchBegin(&kFinger1, kA), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_DOWN, ());
  TEST_EQUAL(r.Last().GetFirstMaskedPointer(), 0, ());

  TEST(r.m_input.TouchBegin(&kFinger2, kB), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_DOWN, ());
  TEST_EQUAL(r.Last().GetFirstMaskedPointer(), 1, ());
  TEST_EQUAL(r.Last().GetSecondTouch().m_location, kB, ());
  auto const id1 = r.Last().GetFirstTouch().m_id;
  auto const id2 = r.Last().GetSecondTouch().m_id;
  TEST_NOT_EQUAL(id1, id2, ());

  // A third finger is ignored.
  int const finger3 = 3;
  TEST(!r.m_input.TouchBegin(&finger3, kC), ());
  TEST(!r.m_input.TouchUpdate(&finger3, kC), ());
  TEST_EQUAL(r.m_events.size(), 2, ());

  TEST(r.m_input.TouchUpdate(&kFinger2, kC), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_MOVE, ());
  TEST_EQUAL(r.Last().GetFirstMaskedPointer(), TouchEvent::INVALID_MASKED_POINTER, ());
  TEST_EQUAL(r.Last().GetSecondTouch().m_location, kC, ());

  // The first finger goes up: the second one keeps its id and becomes the first.
  TEST(r.m_input.TouchEnd(&kFinger1, kA), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_UP, ());
  TEST_EQUAL(r.Last().GetFirstMaskedPointer(), 0, ());
  TEST(r.m_input.TouchUpdate(&kFinger2, kB), ());
  TEST_EQUAL(r.Last().GetFirstTouch().m_id, id2, ());
  TEST_EQUAL(r.Last().GetFirstMaskedPointer(), 0, ());
  TEST(r.m_input.TouchEnd(&kFinger2, kB), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_UP, ());
  TEST(!r.m_input.TouchUpdate(&kFinger2, kB), ());
}

UNIT_TEST(TouchInput_CancelEndsTheGesture)
{
  Recorder r;
  r.m_input.TouchBegin(&kFinger1, kA);
  r.m_input.TouchBegin(&kFinger2, kB);
  TEST(r.m_input.TouchCancel(&kFinger2), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_CANCEL, ());
  TEST_EQUAL(r.m_events.size(), 3, ());

  // The other sequence is cancelled too.
  TEST(!r.m_input.TouchCancel(&kFinger1), ());
  TEST(!r.m_input.TouchUpdate(&kFinger1, kC), ());
  TEST(!r.m_input.TouchEnd(&kFinger1, kC), ());
  TEST_EQUAL(r.m_events.size(), 3, ());

  // Nothing to cancel.
  r.m_input.Cancel();
  TEST_EQUAL(r.m_events.size(), 3, ());
}

UNIT_TEST(TouchInput_Mouse)
{
  Recorder r;
  TEST(!r.m_input.MouseMove(kA), ());
  TEST(!r.m_input.MouseUp(kA), ());
  TEST(r.m_input.MouseDown(kA), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_DOWN, ());
  TEST(r.m_input.MouseMove(kB), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_MOVE, ());
  TEST_EQUAL(r.Last().GetFirstTouch().m_location, kB, ());
  TEST(r.m_input.MouseUp(kC), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_UP, ());
  TEST_EQUAL(r.Last().GetFirstTouch().m_location, kC, ());
  TEST(!r.m_input.MouseMove(kA), ());
  TEST_EQUAL(r.m_events.size(), 3, ());
}

// Hybrid devices: mouse down -> touch begin -> touch cancel -> mouse move/release.
UNIT_TEST(TouchInput_TouchesAreIgnoredWhileMouseIsDown)
{
  Recorder r;
  TEST(r.m_input.MouseDown(kA), ());
  TEST(!r.m_input.TouchBegin(&kFinger1, kB), ());
  TEST(!r.m_input.TouchUpdate(&kFinger1, kB), ());
  TEST(!r.m_input.TouchCancel(&kFinger1), ());
  TEST(!r.m_input.TouchEnd(&kFinger1, kB), ());
  TEST_EQUAL(r.m_events.size(), 1, ());

  TEST(r.m_input.MouseMove(kB), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_MOVE, ());
  TEST(r.m_input.MouseUp(kC), ());
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_UP, ());
  TEST_EQUAL(r.m_events.size(), 3, ());
}

UNIT_TEST(TouchInput_MouseIsIgnoredWhileTouching)
{
  Recorder r;
  TEST(r.m_input.TouchBegin(&kFinger1, kA), ());
  TEST(!r.m_input.MouseDown(kB), ());
  TEST(!r.m_input.MouseMove(kB), ());
  TEST(!r.m_input.MouseUp(kB), ());
  TEST_EQUAL(r.m_events.size(), 1, ());
  TEST(r.m_input.TouchEnd(&kFinger1, kA), ());
  TEST(r.m_input.MouseDown(kB), ());
}

UNIT_TEST(TouchInput_CancelResetsTheMouse)
{
  Recorder r;
  r.m_input.MouseDown(kA);
  r.m_input.Cancel();
  TEST_EQUAL(r.Last().GetTouchType(), TouchEvent::TOUCH_CANCEL, ());
  TEST(!r.m_input.MouseMove(kB), ());
  TEST(!r.m_input.MouseUp(kB), ());
  TEST_EQUAL(r.m_events.size(), 2, ());

  // Both kinds of input work afterwards.
  TEST(r.m_input.TouchBegin(&kFinger1, kA), ());
  TEST(r.m_input.TouchEnd(&kFinger1, kA), ());
  TEST(r.m_input.MouseDown(kA), ());
}
}  // namespace touch_input_tests
