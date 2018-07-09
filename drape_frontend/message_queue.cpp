#include "drape_frontend/message_queue.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

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
  std::unique_lock<std::mutex> lock(m_mutex);
  if (waitForMessage && m_messages.empty() && m_lowPriorityMessages.empty())
  {
    m_isWaiting = true;
    m_condition.wait(lock, [this]() { return !m_isWaiting; });
    m_isWaiting = false;
  }

  if (m_messages.empty() && m_lowPriorityMessages.empty())
    return nullptr;

  if (!m_messages.empty())
  {
    drape_ptr<Message> msg = std::move(m_messages.front().first);
    m_messages.pop_front();
    return msg;
  }

  drape_ptr<Message> msg = std::move(m_lowPriorityMessages.front());
  m_lowPriorityMessages.pop_front();
  return msg;
}

void MessageQueue::PushMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  switch (priority)
  {
  case MessagePriority::Normal:
    {
      m_messages.emplace_back(std::move(message), priority);
      break;
    }
  case MessagePriority::High:
    {
      auto iter = m_messages.begin();
      while (iter != m_messages.end() && iter->second > MessagePriority::High) { iter++; }
      m_messages.emplace(iter, std::move(message), priority);
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
        m_messages.emplace_front(std::move(message), priority);
      break;
    }
  case MessagePriority::Low:
    {
      m_lowPriorityMessages.emplace_back(std::move(message));
      break;
    }
  default:
    ASSERT(false, ("Unknown message priority type"));
  }

  CancelWaitImpl();
}

void MessageQueue::FilterMessages(TFilterMessageFn needFilterMessageFn)
{
  ASSERT(needFilterMessageFn != nullptr, ());

  std::lock_guard<std::mutex> lock(m_mutex);
  for (auto it = m_messages.begin(); it != m_messages.end(); )
  {
    if (needFilterMessageFn(make_ref(it->first)))
      it = m_messages.erase(it);
    else
      ++it;
  }

  for (auto it = m_lowPriorityMessages.begin(); it != m_lowPriorityMessages.end(); )
  {
    if (needFilterMessageFn(make_ref(*it)))
      it = m_lowPriorityMessages.erase(it);
    else
      ++it;
  }
}

#ifdef DEBUG_MESSAGE_QUEUE

bool MessageQueue::IsEmpty() const
{
  lock_guard<mutex> lock(m_mutex);
  return m_messages.empty() && m_lowPriorityMessages.empty();
}

size_t MessageQueue::GetSize() const
{
  lock_guard<mutex> lock(m_mutex);
  return m_messages.size() + m_lowPriorityMessages.size();
}

#endif

void MessageQueue::CancelWait()
{
  std::lock_guard<std::mutex> lock(m_mutex);
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
  m_lowPriorityMessages.clear();
}

} // namespace df
