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
  CancelWaitImpl();
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

  drape_ptr<Message> msg = move(m_messages.front().first);
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
      m_messages.emplace_back(move(message), priority);
      break;
    }
  case MessagePriority::High:
    {
      auto iter = m_messages.begin();
      while (iter != m_messages.end() && iter->second > MessagePriority::High) { iter++; }
      m_messages.emplace(iter, move(message), priority);
      break;
    }
  case MessagePriority::UberHighSingleton:
    {
      bool found = false;
      auto iter = m_messages.begin();
      while (iter != m_messages.end() && iter->second == MessagePriority::UberHighSingleton)
      {
        if (iter->first->GetType() == message->GetType())
        {
          found = true;
          break;
        }
        iter++;
      }

      if (!found)
        m_messages.emplace_front(move(message), priority);
      break;
    }
  default:
    ASSERT(false, ("Unknown message priority type"));
  }

  CancelWaitImpl();
}

void MessageQueue::FilterMessages(TFilterMessageFn needFilterMessageFn)
{
  lock_guard<mutex> lock(m_mutex);
  auto iter = m_messages.begin();
  while (iter != m_messages.end())
  {
    if (needFilterMessageFn(make_ref(iter->first)))
      iter = m_messages.erase(iter);
    else
      ++iter;
  }
}

#ifdef DEBUG_MESSAGE_QUEUE

bool MessageQueue::IsEmpty() const
{
  lock_guard<mutex> lock(m_mutex);
  return m_messages.empty();
}

size_t MessageQueue::GetSize() const
{
  lock_guard<mutex> lock(m_mutex);
  return m_messages.size();
}

#endif

void MessageQueue::CancelWait()
{
  lock_guard<mutex> lock(m_mutex);
  CancelWaitImpl();
}

void MessageQueue::CancelWaitImpl()
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
