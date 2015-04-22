#include "drape_frontend/message_queue.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

namespace df
{

MessageQueue::~MessageQueue()
{
  CancelWait();
  ClearQuery();
}

drape_ptr<Message> MessageQueue::PopMessage(unsigned maxTimeWait)
{
  threads::ConditionGuard guard(m_condition);

  WaitMessage(maxTimeWait);

  /// even waitNonEmpty == true m_messages can be empty after WaitMessage call
  /// if application preparing to close and CancelWait been called
  if (m_messages.empty())
    return nullptr;

  drape_ptr<Message> msg = move(m_messages.front());
  m_messages.pop_front();
  return msg;
}

void MessageQueue::PushMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  threads::ConditionGuard guard(m_condition);

  bool wasEmpty = m_messages.empty();
  switch (priority)
  {
  case MessagePriority::Normal:
    {
      m_messages.emplace_back(move(message));
      break;
    }
  case MessagePriority::High:
    {
      m_messages.emplace_front(move(message));
      break;
    }
  default:
    ASSERT(false, ("Unknown message priority type"));
  }

  if (wasEmpty)
    guard.Signal();
}

bool MessageQueue::IsEmpty()
{
  threads::ConditionGuard guard(m_condition);
  return m_messages.empty();
}

void MessageQueue::WaitMessage(unsigned maxTimeWait)
{
  if (m_messages.empty())
    m_condition.Wait(maxTimeWait);
}

void MessageQueue::CancelWait()
{
  m_condition.Signal();
}

void MessageQueue::ClearQuery()
{
  m_messages.clear();
}

} // namespace df
