#pragma once

#include "drape_frontend/message_queue.hpp"

#include "drape/pointers.hpp"

namespace df
{

class Message;

class MessageAcceptor
{
public:
  virtual ~MessageAcceptor() {}

protected:
  virtual void AcceptMessage(dp::RefPointer<Message> message) = 0;

  /// Must be called by subclass on message target thread
  void ProcessSingleMessage(unsigned maxTimeWait = -1);
  void CloseQueue();

private:
  friend class ThreadsCommutator;

  void PostMessage(dp::TransferPointer<Message> message, MessagePriority priority);

private:
  MessageQueue m_messageQueue;
};

} // namespace df
