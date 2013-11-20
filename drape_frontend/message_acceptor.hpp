#pragma once

#include "message_queue.hpp"

namespace df
{
  class Message;

  class MessageAcceptor
  {
  protected:
    virtual void AcceptMessage(Message * message) = 0;

    /// Must be called by subclass on message target thread
    void ProcessSingleMessage(bool waitMessage);
    void CloseQueue();

  private:
    friend class ThreadsCommutator;

    void PostMessage(Message * message);
    void ClearQueue();

  private:
    MessageQueue m_messageQueue;
  };
}
