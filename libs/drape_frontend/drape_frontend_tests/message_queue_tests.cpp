#include "testing/testing.hpp"

#include "drape_frontend/message_queue.hpp"

#ifdef DRAPE_QUEUE_TRACE
#include "drape_frontend/message_acceptor.hpp"

#include <limits>
#endif

#include <array>
#include <chrono>
#include <future>

namespace message_queue_tests
{
using namespace std::chrono_literals;

class TestMessage : public df::Message
{
public:
  TestMessage(int id, Type type, int * destroyed = nullptr) : m_id(id), m_type(type), m_destroyed(destroyed) {}
  ~TestMessage() override
  {
    if (m_destroyed)
      ++*m_destroyed;
  }
  Type GetType() const override { return m_type; }

  int const m_id;

private:
  Type const m_type;
  int * m_destroyed;
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

UNIT_TEST(MessageQueue_TimedWaitExpires)
{
  df::MessageQueue queue;
  auto result = std::async(std::launch::async, [&queue] { return queue.PopMessage(true, 10ms); });
  TEST(WaitForResult(queue, result) == nullptr, ());

  queue.PushMessage(make_unique_dp<TestMessage>(1, df::Message::Type::Invalidate), df::MessagePriority::Low);
  auto const msg = queue.PopMessage(true, 10ms);
  TEST(msg != nullptr, ());
  TEST_EQUAL(static_cast<TestMessage *>(msg.get())->m_id, 1, ());
}

UNIT_TEST(MessageQueue_CancelBeforeTimedWait)
{
  df::MessageQueue queue;
  queue.CancelWait();
  auto result = std::async(std::launch::async, [&queue] { return queue.PopMessage(true, 10s); });
  TEST(WaitForResult(queue, result) == nullptr, ());
}

UNIT_TEST(MessageQueue_CancelRacingWithTimedWait)
{
  df::MessageQueue queue;
  std::promise<void> started;
  auto result = std::async(std::launch::async, [&queue, &started]
  {
    started.set_value();
    return queue.PopMessage(true, 10s);
  });
  started.get_future().wait();
  queue.CancelWait();
  TEST(WaitForResult(queue, result) == nullptr, ());
}

UNIT_TEST(MessageQueue_LowPriorityPushRacingWithTimedWait)
{
  df::MessageQueue queue;
  std::promise<void> started;
  auto result = std::async(std::launch::async, [&queue, &started]
  {
    started.set_value();
    return queue.PopMessage(true, 10s);
  });
  started.get_future().wait();
  queue.PushMessage(make_unique_dp<TestMessage>(1, df::Message::Type::Invalidate), df::MessagePriority::Low);
  auto const msg = WaitForResult(queue, result);
  TEST(msg != nullptr, ());
  TEST_EQUAL(static_cast<TestMessage *>(msg.get())->m_id, 1, ());
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

UNIT_TEST(MessageQueue_FilterMixedPriorities)
{
  using Type = df::Message::Type;
  std::array<int, 13> destructionCounts = {};
  std::array<int, 13> filterCounts = {};
  df::MessageQueue queue;
  auto push = [&](int id, df::MessagePriority priority, Type type = Type::Invalidate)
  { queue.PushMessage(make_unique_dp<TestMessage>(id, type, &destructionCounts[id]), priority); };
  for (int id = 1; id <= 4; ++id)
    push(id, df::MessagePriority::Normal);
  for (int id = 5; id <= 6; ++id)
    push(id, df::MessagePriority::High);
  push(7, df::MessagePriority::UberHighSingleton);
  push(8, df::MessagePriority::UberHighSingleton, Type::UpdateReadManager);
  for (int id = 9; id <= 12; ++id)
    push(id, df::MessagePriority::Low);

  queue.InstantFilter([&](ref_ptr<df::Message> message)
  {
    auto const id = static_cast<TestMessage *>(message.get())->m_id;
    ++filterCounts[id];
    return id % 2 != 0;
  });

  for (int id = 1; id <= 12; ++id)
  {
    TEST_EQUAL(filterCounts[id], 1, (id));
    TEST_EQUAL(destructionCounts[id], id % 2, (id));
  }
  for (int const id : {8, 6, 2, 4, 10, 12})
    TEST_EQUAL(PopId(queue), id, ());
  TEST(queue.PopMessage(false) == nullptr, ());
  for (int id = 1; id <= 12; ++id)
    TEST_EQUAL(destructionCounts[id], 1, (id));
}
#ifdef DRAPE_QUEUE_TRACE
UNIT_TEST(MessageQueue_TraceCountsAllRemovalAndRejectionPaths)
{
  using Type = df::Message::Type;
  df::MessageQueue queue;
  queue.PushMessage(make_unique_dp<TestMessage>(1, Type::Invalidate), df::MessagePriority::Normal);
  queue.PushMessage(make_unique_dp<TestMessage>(2, Type::Invalidate), df::MessagePriority::Low);
  queue.PushMessage(make_unique_dp<TestMessage>(3, Type::FlushTile), df::MessagePriority::High);
  queue.PushMessage(make_unique_dp<TestMessage>(4, Type::UpdateReadManager), df::MessagePriority::UberHighSingleton);
  queue.PushMessage(make_unique_dp<TestMessage>(5, Type::UpdateReadManager), df::MessagePriority::UberHighSingleton);
  queue.EnableMessageFiltering([](ref_ptr<df::Message> m) { return m->GetType() == Type::Invalidate; });
  queue.PushMessage(make_unique_dp<TestMessage>(6, Type::Invalidate), df::MessagePriority::Normal);
  TEST_EQUAL(PopId(queue), 4, ());
  queue.Clear();

  auto const trace = queue.GetTrace();
  TEST_EQUAL(trace.m_size, 0, ());
  TEST_EQUAL(trace.m_peak, 4, ("Singleton rejection must not inflate the peak"));
  auto const & filtered = trace.m_types.at(Type::Invalidate);
  TEST_EQUAL(filtered.m_enqueued, 2, ());
  TEST_EQUAL(filtered.m_filtered, 2, ("Both normal and low priority queues are counted"));
  TEST_EQUAL(filtered.m_rejected, 1, ());
  TEST_EQUAL(filtered.m_peak, 2, ());
  auto const & popped = trace.m_types.at(Type::UpdateReadManager);
  TEST_EQUAL(popped.m_enqueued, 1, ());
  TEST_EQUAL(popped.m_popped, 1, ());
  TEST_EQUAL(popped.m_rejected, 1, ());
  TEST_EQUAL(popped.m_peak, 1, ());
  TEST_EQUAL(trace.m_types.at(Type::FlushTile).m_cleared, 1, ());
  for (auto const & [type, counts] : trace.m_types)
  {
    TEST_EQUAL(counts.m_size, 0, (static_cast<int>(type)));
    TEST_EQUAL(counts.m_enqueued, counts.m_size + counts.m_popped + counts.m_filtered + counts.m_cleared,
               (static_cast<int>(type)));
  }
}

UNIT_TEST(MessageQueue_TraceRecordsAreBoundedAndComplete)
{
  df::MessageQueue::TraceSnapshot trace;
  auto const empty = df::FormatMessageQueueTrace(trace, "backend", 1);
  TEST_EQUAL(empty.size(), 1, ());
  TEST_EQUAL(
      empty.front(),
      "DrapeQueue {\"renderer\":\"backend\",\"size\":0,\"peak\":0,\"sample\":1,\"part\":0,\"parts\":1,\"types\":{}}",
      ());

  auto const max = std::numeric_limits<uint64_t>::max();
  trace.m_size = trace.m_peak = std::numeric_limits<size_t>::max();
  for (int i = 0; i <= static_cast<int>(df::Message::Type::AssignTileBackgroundImage); ++i)
  {
    auto & counts = trace.m_types[static_cast<df::Message::Type>(i)];
    counts.m_size = counts.m_peak = std::numeric_limits<size_t>::max();
    counts.m_enqueued = counts.m_popped = counts.m_filtered = counts.m_rejected = counts.m_cleared = max;
  }

  auto const parts = df::FormatMessageQueueTrace(trace, "frontend", max);
  TEST_GREATER(parts.size(), 1, ());
  for (size_t i = 0; i < parts.size(); ++i)
  {
    TEST_LESS_OR_EQUAL(parts[i].size(), 3000, ());
    auto const metadata = "\"sample\":" + std::to_string(max) + ",\"part\":" + std::to_string(i) +
                          ",\"parts\":" + std::to_string(parts.size());
    TEST(parts[i].find(metadata) != std::string::npos, (parts[i]));
  }
  for (auto const & [type, counts] : trace.m_types)
  {
    auto const key = '"' + std::string(DebugPrint(type)) + "\":{";
    size_t found = 0;
    for (auto const & part : parts)
      found += part.find(key) != std::string::npos;
    TEST_EQUAL(found, 1, (static_cast<int>(type)));
  }
}
#endif
}  // namespace message_queue_tests
