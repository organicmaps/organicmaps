#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

#include <atomic>
#include <functional>

namespace df
{

class Message;

class MessageAcceptor
{
protected:
  MessageAcceptor();
  virtual ~MessageAcceptor(){}

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

  using TFilterMessageFn = std::function<bool(ref_ptr<Message>)>;
  void EnableMessageFiltering(TFilterMessageFn needFilterMessageFn);
  void DisableMessageFiltering();

private:
  friend class ThreadsCommutator;

  void PostMessage(drape_ptr<Message> && message, MessagePriority priority);

  MessageQueue m_messageQueue;
  std::atomic<bool> m_infinityWaiting;
  TFilterMessageFn m_needFilterMessageFn;
};

} // namespace df
