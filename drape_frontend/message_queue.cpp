#include "drape_frontend/message_queue.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include "std/chrono.hpp"

namespace df
{

MessageQueue::MessageQueue()
  : m_isWaiting(false)
{
}

MessageQueue::~MessageQueue()
{
  CancelWait();
  ClearQuery();
}

drape_ptr<Message> MessageQueue::PopMessage(bool waitForMessage)
{
  unique_lock<mutex> lock(m_mutex);
  if (waitForMessage && m_messages.empty())
  {
    m_isWaiting = true;
    m_condition.wait(lock, [this]() { return !m_isWaiting; });
    m_isWaiting = false;
  }

  if (m_messages.empty())
    return nullptr;

  drape_ptr<Message> msg = move(m_messages.front());
  m_messages.pop_front();
  return msg;
}

void MessageQueue::PushMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  lock_guard<mutex> lock(m_mutex);

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

  CancelWait();
}

bool MessageQueue::IsEmpty()
{
  lock_guard<mutex> lock(m_mutex);
  return m_messages.empty();
}

void MessageQueue::CancelWait()
{
  if (m_isWaiting)
  {
    m_isWaiting = false;
    m_condition.notify_all();
  }
}

void MessageQueue::ClearQuery()
{
  m_messages.clear();
}

} // namespace df
