#pragma once

#include "drape_frontend/message.hpp"

#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

#ifdef DRAPE_QUEUE_TRACE
#include <cstdint>
#include <map>
#endif

namespace df
{
// The queue has a single consumer: only one thread may call PopMessage(), since one cancellation
// flag and notify_one() cannot serve several waiters.
class MessageQueue
{
public:
  // Returns the highest priority message, or nullptr if the queue is empty and waitForMessage is
  // false, or if the wait was interrupted by CancelWait(). A queued message wins over a pending
  // cancellation and consumes it, so a cancelled wait is not guaranteed to be observed as a nullptr.
  drape_ptr<Message> PopMessage(bool waitForMessage);
  void PushMessage(drape_ptr<Message> && message, MessagePriority priority);
  // Interrupts the current or the next PopMessage(true). A PopMessage(false) leaves it pending.
  void CancelWait();
  void Clear();

  using FilterMessageFn = std::function<bool(ref_ptr<Message>)>;
  void EnableMessageFiltering(FilterMessageFn && filter);
  void DisableMessageFiltering();
  void InstantFilter(FilterMessageFn && filter);

#ifdef DEBUG_MESSAGE_QUEUE
  bool IsEmpty() const;
  size_t GetSize() const;
#endif

#ifdef DRAPE_QUEUE_TRACE
  struct TraceCounts
  {
    size_t m_size = 0;
    size_t m_peak = 0;
    uint64_t m_enqueued = 0;
    uint64_t m_popped = 0;
    uint64_t m_filtered = 0;
    uint64_t m_rejected = 0;
    uint64_t m_cleared = 0;
  };

  struct TraceSnapshot
  {
    size_t m_size = 0;
    size_t m_peak = 0;
    std::map<Message::Type, TraceCounts> m_types;
  };

  TraceSnapshot GetTrace() const;
#endif

private:
  void FilterMessagesImpl();

#ifdef DRAPE_QUEUE_TRACE
  void TraceEnqueued(Message::Type type);
  void TraceRemoved(Message::Type type, uint64_t TraceCounts::* counter);
  TraceSnapshot m_trace;
#endif

  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  // Makes a cancellation observable even if it precedes PopMessage(). Consumed by every
  // PopMessage(true), including one that returns a queued message without waiting.
  bool m_cancelPending = false;
  using TMessageNode = std::pair<drape_ptr<Message>, MessagePriority>;
  std::deque<TMessageNode> m_messages;
  std::deque<drape_ptr<Message>> m_lowPriorityMessages;
  FilterMessageFn m_filter;
};
}  // namespace df
