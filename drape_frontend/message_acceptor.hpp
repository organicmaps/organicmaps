#pragma once

#include "message_queue.hpp"

#include "../drape/pointers.hpp"

namespace df
{
  class Message;

  class MessageAcceptor
  {
  protected:
    virtual void AcceptMessage(RefPointer<Message> message) = 0;

    /// Must be called by subclass on message target thread
    void ProcessSingleMessage(unsigned maxTimeWait = -1);
    void CloseQueue();

  private:
    friend class ThreadsCommutator;

    void PostMessage(TransferPointer<Message> message);

  private:
    MessageQueue m_messageQueue;
  };
}
