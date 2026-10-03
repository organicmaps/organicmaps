#include "drape_frontend/message_queue.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
drape_ptr<Message> MessageQueue::PopMessage(bool waitForMessage)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (waitForMessage)
  {
    m_condition.wait(lock, [this] { return m_cancelPending || !m_messages.empty() || !m_lowPriorityMessages.empty(); });
    m_cancelPending = false;
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
#ifdef DRAPE_QUEUE_TRACE
  if (msg)
    TraceRemoved(msg->GetType(), &TraceCounts::m_popped);
#endif
  return msg;
}

void MessageQueue::PushMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_filter != nullptr && m_filter(make_ref(message)))
  {
#ifdef DRAPE_QUEUE_TRACE
    ++m_trace.m_types[message->GetType()].m_rejected;
#endif
    return;
  }

#ifdef DRAPE_QUEUE_TRACE
  auto const type = message->GetType();
  auto const sizeBefore = m_messages.size() + m_lowPriorityMessages.size();
#endif

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

#ifdef DRAPE_QUEUE_TRACE
  if (m_messages.size() + m_lowPriorityMessages.size() != sizeBefore)
    TraceEnqueued(type);
  else
    ++m_trace.m_types[type].m_rejected;
#endif

  m_condition.notify_one();
}

void MessageQueue::FilterMessagesImpl()
{
  CHECK(m_filter != nullptr, ());

  auto const filter = [this](auto const & message)
  {
    bool const remove = m_filter(make_ref(message));
#ifdef DRAPE_QUEUE_TRACE
    if (remove)
      TraceRemoved(message->GetType(), &TraceCounts::m_filtered);
#endif
    return remove;
  };
  std::erase_if(m_messages, [&filter](auto const & message) { return filter(message.first); });
  std::erase_if(m_lowPriorityMessages, filter);
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
  m_cancelPending = true;
  m_condition.notify_one();
}

void MessageQueue::Clear()
{
  std::lock_guard<std::mutex> lock(m_mutex);
#ifdef DRAPE_QUEUE_TRACE
  for (auto const & node : m_messages)
    TraceRemoved(node.first->GetType(), &TraceCounts::m_cleared);
  for (auto const & message : m_lowPriorityMessages)
    TraceRemoved(message->GetType(), &TraceCounts::m_cleared);
#endif
  m_messages.clear();
  m_lowPriorityMessages.clear();
}

#ifdef DRAPE_QUEUE_TRACE
void MessageQueue::TraceEnqueued(Message::Type type)
{
  auto & counts = m_trace.m_types[type];
  ++counts.m_enqueued;
  counts.m_peak = std::max(counts.m_peak, ++counts.m_size);
  m_trace.m_peak = std::max(m_trace.m_peak, ++m_trace.m_size);
}

void MessageQueue::TraceRemoved(Message::Type type, uint64_t TraceCounts::* counter)
{
  auto & counts = m_trace.m_types.at(type);
  ++(counts.*counter);
  --counts.m_size;
  --m_trace.m_size;
}

MessageQueue::TraceSnapshot MessageQueue::GetTrace() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_trace;
}
#endif
}  // namespace df
