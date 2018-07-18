#pragma once

#include "drape_frontend/message.hpp"

#include "drape/pointers.hpp"

#include "base/condition.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

namespace df
{
//#define DEBUG_MESSAGE_QUEUE

class MessageQueue
{
public:
  MessageQueue();
  ~MessageQueue();

  /// if queue is empty then return NULL
  drape_ptr<Message> PopMessage(bool waitForMessage);
  void PushMessage(drape_ptr<Message> && message, MessagePriority priority);
  void CancelWait();
  void ClearQuery();

  using FilterMessageFn = std::function<bool(ref_ptr<Message>)>;
  void EnableMessageFiltering(FilterMessageFn && filter);
  void DisableMessageFiltering();

#ifdef DEBUG_MESSAGE_QUEUE
  bool IsEmpty() const;
  size_t GetSize() const;
#endif

private:
  void FilterMessagesImpl();
  void CancelWaitImpl();

  mutable std::mutex m_mutex;
  std::condition_variable m_condition;
  bool m_isWaiting;
  using TMessageNode = std::pair<drape_ptr<Message>, MessagePriority>;
  std::deque<TMessageNode> m_messages;
  std::deque<drape_ptr<Message>> m_lowPriorityMessages;
  FilterMessageFn m_filter;
};
}  // namespace df
