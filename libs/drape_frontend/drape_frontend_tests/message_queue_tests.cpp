#include "testing/testing.hpp"

#include "drape_frontend/message_queue.hpp"

#include <chrono>
#include <future>

namespace message_queue_tests
{
using namespace std::chrono_literals;

class TestMessage : public df::Message
{
public:
  TestMessage(int id, Type type) : m_id(id), m_type(type) {}
  Type GetType() const override { return m_type; }

  int const m_id;

private:
  Type const m_type;
};

int PopId(df::MessageQueue & queue)
{
  auto const msg = queue.PopMessage(false /* waitForMessage */);
  TEST(msg != nullptr, ());
  return static_cast<TestMessage *>(msg.get())->m_id;
}

drape_ptr<df::Message> WaitForResult(df::MessageQueue & queue, std::future<drape_ptr<df::Message>> & result)
{
  auto const status = result.wait_for(5s);
  if (status != std::future_status::ready)
    queue.CancelWait();
  TEST(status == std::future_status::ready, ());
  return result.get();
}

UNIT_TEST(MessageQueue_CancelBeforeWait)
{
  df::MessageQueue queue;
  queue.CancelWait();

  auto result = std::async(std::launch::async, [&queue] { return queue.PopMessage(true); });
  TEST(WaitForResult(queue, result) == nullptr, ());
}

UNIT_TEST(MessageQueue_CancelSurvivesNonBlockingPop)
{
  df::MessageQueue queue;
  queue.CancelWait();
  TEST(queue.PopMessage(false) == nullptr, ());

  auto result = std::async(std::launch::async, [&queue] { return queue.PopMessage(true); });
  TEST(WaitForResult(queue, result) == nullptr, ());
}

// A queued message wins over a pending cancellation and consumes it: the wait did end, so callers must
// not expect the cancellation to resurface as a later nullptr.
UNIT_TEST(MessageQueue_MessageWinsOverPendingCancel)
{
  df::MessageQueue queue;
  queue.PushMessage(make_unique_dp<TestMessage>(1, df::Message::Type::Invalidate), df::MessagePriority::Normal);
  queue.CancelWait();

  auto const msg = queue.PopMessage(true);
  TEST(msg != nullptr, ());
  TEST_EQUAL(static_cast<TestMessage *>(msg.get())->m_id, 1, ());

  auto result = std::async(std::launch::async, [&queue] { return queue.PopMessage(true); });
  TEST(result.wait_for(200ms) == std::future_status::timeout, ());
  queue.CancelWait();
  TEST(WaitForResult(queue, result) == nullptr, ());
}

UNIT_TEST(MessageQueue_CancelRacingWithWait)
{
  df::MessageQueue queue;
  std::promise<void> started;
  auto result = std::async(std::launch::async, [&queue, &started]
  {
    started.set_value();
    return queue.PopMessage(true);
  });
  started.get_future().wait();
  queue.CancelWait();

  TEST(WaitForResult(queue, result) == nullptr, ());
}

UNIT_TEST(MessageQueue_PushRacingWithWait)
{
  df::MessageQueue queue;
  std::promise<void> started;
  auto result = std::async(std::launch::async, [&queue, &started]
  {
    started.set_value();
    return queue.PopMessage(true);
  });
  started.get_future().wait();
  queue.PushMessage(make_unique_dp<df::Message>(), df::MessagePriority::Normal);

  TEST(WaitForResult(queue, result) != nullptr, ());
}

// UberHighSingleton jumps the whole queue and is deduplicated by message type, High overtakes Normal but
// stays behind UberHighSingleton, and Low is drained only after everything else.
UNIT_TEST(MessageQueue_PriorityOrder)
{
  using Type = df::Message::Type;
  df::MessageQueue queue;
  queue.PushMessage(make_unique_dp<TestMessage>(1, Type::Invalidate), df::MessagePriority::Normal);
  queue.PushMessage(make_unique_dp<TestMessage>(2, Type::Invalidate), df::MessagePriority::Low);
  queue.PushMessage(make_unique_dp<TestMessage>(3, Type::Invalidate), df::MessagePriority::High);
  queue.PushMessage(make_unique_dp<TestMessage>(4, Type::UpdateReadManager), df::MessagePriority::UberHighSingleton);
  queue.PushMessage(make_unique_dp<TestMessage>(5, Type::FlushTile), df::MessagePriority::UberHighSingleton);
  queue.PushMessage(make_unique_dp<TestMessage>(6, Type::UpdateReadManager), df::MessagePriority::UberHighSingleton);

  TEST_EQUAL(PopId(queue), 5, ());
  TEST_EQUAL(PopId(queue), 4, ());
  TEST_EQUAL(PopId(queue), 3, ());
  TEST_EQUAL(PopId(queue), 1, ());
  TEST_EQUAL(PopId(queue), 2, ());
  TEST(queue.PopMessage(false) == nullptr, ());
}

// Filtering drops matching messages both from the queue and on arrival, until it is disabled;
// InstantFilter makes a single pass and leaves no filter installed.
UNIT_TEST(MessageQueue_Filtering)
{
  using Type = df::Message::Type;
  auto isInvalidate = [](ref_ptr<df::Message> m) { return m->GetType() == Type::Invalidate; };

  df::MessageQueue queue;
  queue.PushMessage(make_unique_dp<TestMessage>(1, Type::Invalidate), df::MessagePriority::Normal);
  queue.PushMessage(make_unique_dp<TestMessage>(2, Type::FlushTile), df::MessagePriority::Normal);

  queue.EnableMessageFiltering(isInvalidate);
  queue.PushMessage(make_unique_dp<TestMessage>(3, Type::Invalidate), df::MessagePriority::Normal);
  TEST_EQUAL(PopId(queue), 2, ());
  TEST(queue.PopMessage(false) == nullptr, ());

  queue.DisableMessageFiltering();
  queue.PushMessage(make_unique_dp<TestMessage>(4, Type::Invalidate), df::MessagePriority::Normal);
  queue.InstantFilter(isInvalidate);
  queue.PushMessage(make_unique_dp<TestMessage>(5, Type::Invalidate), df::MessagePriority::Normal);
  TEST_EQUAL(PopId(queue), 5, ());
}
}  // namespace message_queue_tests
