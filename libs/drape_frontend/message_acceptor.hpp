#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

namespace df
{
class Message;

class MessageAcceptor
{
protected:
  MessageAcceptor() = default;
  virtual ~MessageAcceptor() = default;

  virtual void AcceptMessage(ref_ptr<Message> message) = 0;

  /// Must be called by subclass on message target thread
  bool ProcessSingleMessage(bool waitForMessage = true);

  void CancelMessageWaiting();

  void CloseQueue();

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
};
}  // namespace df
