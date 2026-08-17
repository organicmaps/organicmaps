#pragma once

#include "drape_frontend/message.hpp"

#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

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

private:
  void FilterMessagesImpl();

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
