#pragma once

#include "drape_frontend/message.hpp"

#include "drape/pointers.hpp"

#include "base/condition.hpp"

#include "std/deque.hpp"

namespace df
{

class MessageQueue
{
public:
  ~MessageQueue();

  /// if queue is empty then return NULL
  dp::TransferPointer<Message> PopMessage(unsigned maxTimeWait);
  void PushMessage(dp::TransferPointer<Message> message, MessagePriority priority);
  void CancelWait();
  void ClearQuery();

private:
  void WaitMessage(unsigned maxTimeWait);

private:
  threads::Condition m_condition;
  deque<dp::MasterPointer<Message> > m_messages;
};

} // namespace df
