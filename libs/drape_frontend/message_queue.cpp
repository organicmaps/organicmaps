#include "drape_frontend/message_queue.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

namespace df
{
MessageQueue::MessageQueue() : m_isWaiting(false) {}

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

  drape_ptr<Message> msg;
  if (!m_messages.empty())
  {
    msg = std::move(m_messages.front().first);
    m_messages.pop_front();
  }
  else if (!m_lowPriorityMessages.empty())
  {
    msg = std::move(m_lowPriorityMessages.front());
    m_lowPriorityMessages.pop_front();
  }
  return msg;
}

void MessageQueue::PushMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_filter != nullptr && m_filter(make_ref(message)))
    return;

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
    while (iter != m_messages.end() && iter->second > MessagePriority::High)
      iter++;
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
  default: ASSERT(false, ("Unknown message priority type"));
  }

  CancelWaitImpl();
}

void MessageQueue::FilterMessagesImpl()
{
  CHECK(m_filter != nullptr, ());

  for (auto it = m_messages.begin(); it != m_messages.end();)
    if (m_filter(make_ref(it->first)))
      it = m_messages.erase(it);
    else
      ++it;

  for (auto it = m_lowPriorityMessages.begin(); it != m_lowPriorityMessages.end();)
    if (m_filter(make_ref(*it)))
      it = m_lowPriorityMessages.erase(it);
    else
      ++it;
}

void MessageQueue::EnableMessageFiltering(FilterMessageFn && filter)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_filter = std::move(filter);
  FilterMessagesImpl();
}

void MessageQueue::DisableMessageFiltering()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_filter = nullptr;
}

void MessageQueue::InstantFilter(FilterMessageFn && filter)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  CHECK(m_filter == nullptr, ());
  m_filter = std::move(filter);
  FilterMessagesImpl();
  m_filter = nullptr;
}

#ifdef DEBUG_MESSAGE_QUEUE
bool MessageQueue::IsEmpty() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_messages.empty() && m_lowPriorityMessages.empty();
}

size_t MessageQueue::GetSize() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
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
}  // namespace df
