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
  drape_ptr<Message> PopMessage(unsigned maxTimeWait);
  void PushMessage(drape_ptr<Message> && message, MessagePriority priority);
  void CancelWait();
  void ClearQuery();
  bool IsEmpty();

private:
  void WaitMessage(unsigned maxTimeWait);

private:
  threads::Condition m_condition;
  deque<drape_ptr<Message> > m_messages;
};

} // namespace df
