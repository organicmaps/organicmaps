#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

#ifdef DRAPE_QUEUE_TRACE
#include "base/timer.hpp"

#include <string>
#include <string_view>
#include <vector>
#endif

#include <chrono>

namespace df
{
class Message;

#ifdef DRAPE_QUEUE_TRACE
// Keep individual log records below Android's logcat limit while preserving a complete snapshot.
std::vector<std::string> FormatMessageQueueTrace(MessageQueue::TraceSnapshot const & trace, std::string_view renderer,
                                                 uint64_t sample);
#endif

class MessageAcceptor
{
protected:
  MessageAcceptor() = default;
  virtual ~MessageAcceptor() = default;

  virtual void AcceptMessage(ref_ptr<Message> message) = 0;

  /// Must be called by subclass on message target thread
  bool ProcessSingleMessage(bool waitForMessage = true,
                            std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

  void CancelMessageWaiting();

  void CloseQueue();

#ifdef DEBUG_MESSAGE_QUEUE
  bool IsQueueEmpty() const;
  size_t GetQueueSize() const;
#endif

  void EnableMessageFiltering(MessageQueue::FilterMessageFn && filter);
  void DisableMessageFiltering();
  void InstantMessageFilter(MessageQueue::FilterMessageFn && filter);

#ifdef DRAPE_QUEUE_TRACE
  // Called on the receiving thread; reports cumulative counters at most once per second.
  void TraceMessageQueue(std::string_view renderer);
#endif

private:
  friend class ThreadsCommutator;

  void PostMessage(drape_ptr<Message> && message, MessagePriority priority);

  MessageQueue m_messageQueue;
#ifdef DRAPE_QUEUE_TRACE
  base::Timer m_queueTraceTimer;
  uint64_t m_queueTraceSample = 0;
#endif
};
}  // namespace df
