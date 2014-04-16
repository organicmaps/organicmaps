#pragma once

#include "message.hpp"

#include "../drape/pointers.hpp"

#include "../base/condition.hpp"

#include "../std/list.hpp"

namespace df
{

class MessageQueue
{
public:
  ~MessageQueue();

  /// if queue is empty than return NULL
  TransferPointer<Message> PopMessage(unsigned maxTimeWait);
  void PushMessage(TransferPointer<Message> message);
  void CancelWait();
  void ClearQuery();

private:
  void WaitMessage(unsigned maxTimeWait);

private:
  threads::Condition m_condition;
  list<MasterPointer<Message> > m_messages;
};

} // namespace df
