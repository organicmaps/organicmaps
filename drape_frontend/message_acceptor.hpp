#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

#include "std/atomic.hpp"

namespace df
{

class Message;

class MessageAcceptor
{
protected:
  MessageAcceptor();
  virtual ~MessageAcceptor(){}

  virtual void AcceptMessage(ref_ptr<Message> message) = 0;
  virtual bool CanReceiveMessage() = 0;

  /// Must be called by subclass on message target thread
  bool ProcessSingleMessage(bool waitForMessage = true);

  void CancelMessageWaiting();

  void CloseQueue();

  bool IsInInfinityWaiting() const;
  bool IsQueueEmpty() const;
  bool GetQueueSize() const;

private:
  friend class ThreadsCommutator;

  void PostMessage(drape_ptr<Message> && message, MessagePriority priority);

  MessageQueue m_messageQueue;
  atomic<bool> m_infinityWaiting;
};

} // namespace df
