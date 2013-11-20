#pragma once

#include "message.hpp"

#include "../base/condition.hpp"

#include "../std/list.hpp"

namespace df
{
  class MessageQueue
  {
  public:
    MessageQueue();

    /// if queue is empty than return NULL
    Message * PopMessage(bool waitMessage);
    void PushMessage(Message * message);
    void CancelWait();

  private:
    void WaitMessage();

  private:
    threads::Condition m_condition;
    list<Message *> m_messages;
  };
}
