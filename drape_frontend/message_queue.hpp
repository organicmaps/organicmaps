#pragma once

#include "drape_frontend/message.hpp"

#include "drape/pointers.hpp"

#include "base/condition.hpp"

#include "std/condition_variable.hpp"
#include "std/deque.hpp"
#include "std/mutex.hpp"

namespace df
{

class MessageQueue
{
public:
  MessageQueue();
  ~MessageQueue();

  /// if queue is empty then return NULL
  drape_ptr<Message> PopMessage(unsigned maxTimeWait);
  void PushMessage(drape_ptr<Message> && message, MessagePriority priority);
  void CancelWait();
  void ClearQuery();
  bool IsEmpty();

private:
  mutex m_mutex;
  condition_variable m_condition;
  bool m_isWaiting;
  deque<drape_ptr<Message> > m_messages;
};

} // namespace df
