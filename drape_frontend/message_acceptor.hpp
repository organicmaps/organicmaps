#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

#include <atomic>
#include <functional>
#include <mutex>

namespace df
{
class Message;

class MessageAcceptor
{
protected:
  MessageAcceptor();
  virtual ~MessageAcceptor() = default;

  virtual void AcceptMessage(ref_ptr<Message> message) = 0;

  /// Must be called by subclass on message target thread
  bool ProcessSingleMessage(bool waitForMessage = true);

  void CancelMessageWaiting();

  void CloseQueue();

  bool IsInInfinityWaiting() const;

#ifdef DEBUG_MESSAGE_QUEUE
  bool IsQueueEmpty() const;
  size_t GetQueueSize() const;
#endif

  void EnableMessageFiltering(MessageQueue::FilterMessageFn && filter);
  void DisableMessageFiltering();
  void InstantMessageFilter(MessageQueue::FilterMessageFn && filter);

private:
  friend class ThreadsCommutator;

  void PostMessage(drape_ptr<Message> && message, MessagePriority priority);

  MessageQueue m_messageQueue;
  std::atomic<bool> m_infinityWaiting;
};
}  // namespace df
