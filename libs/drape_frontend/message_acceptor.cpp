#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

#ifdef DRAPE_QUEUE_TRACE
#include "base/logging.hpp"

#include <sstream>
#endif

namespace df
{
bool MessageAcceptor::ProcessSingleMessage(bool waitForMessage, std::chrono::milliseconds timeout)
{
  drape_ptr<Message> message = m_messageQueue.PopMessage(waitForMessage, timeout);
  if (message == nullptr)
    return false;

  AcceptMessage(make_ref(message));
  return true;
}

void MessageAcceptor::EnableMessageFiltering(MessageQueue::FilterMessageFn && filter)
{
  m_messageQueue.EnableMessageFiltering(std::move(filter));
}

void MessageAcceptor::DisableMessageFiltering()
{
  m_messageQueue.DisableMessageFiltering();
}

void MessageAcceptor::InstantMessageFilter(MessageQueue::FilterMessageFn && filter)
{
  m_messageQueue.InstantFilter(std::move(filter));
}

void MessageAcceptor::PostMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  m_messageQueue.PushMessage(std::move(message), priority);
}

void MessageAcceptor::CloseQueue()
{
  m_messageQueue.CancelWait();
  m_messageQueue.Clear();
}

void MessageAcceptor::CancelMessageWaiting()
{
  m_messageQueue.CancelWait();
}

#ifdef DRAPE_QUEUE_TRACE
std::vector<std::string> FormatMessageQueueTrace(MessageQueue::TraceSnapshot const & trace, std::string_view renderer,
                                                 uint64_t sample)
{
  // Leave room for the snapshot header and the platform logger's prefix.
  size_t constexpr kMaxTypeBytes = 2500;
  std::vector<std::string> parts(1);
  for (auto const & [type, counts] : trace.m_types)
  {
    std::ostringstream entry;
    entry << '"' << DebugPrint(type) << "\":{\"size\":" << counts.m_size << ",\"peak\":" << counts.m_peak
          << ",\"enqueued\":" << counts.m_enqueued << ",\"popped\":" << counts.m_popped
          << ",\"filtered\":" << counts.m_filtered << ",\"rejected\":" << counts.m_rejected
          << ",\"cleared\":" << counts.m_cleared << '}';
    auto const value = entry.str();
    if (!parts.back().empty() && parts.back().size() + value.size() + 1 > kMaxTypeBytes)
      parts.emplace_back();
    if (!parts.back().empty())
      parts.back() += ',';
    parts.back() += value;
  }

  for (size_t i = 0; i < parts.size(); ++i)
  {
    std::ostringstream out;
    out << "DrapeQueue {\"renderer\":\"" << renderer << "\",\"size\":" << trace.m_size << ",\"peak\":" << trace.m_peak
        << ",\"sample\":" << sample << ",\"part\":" << i << ",\"parts\":" << parts.size() << ",\"types\":{" << parts[i]
        << "}}";
    parts[i] = out.str();
    ASSERT_LESS_OR_EQUAL(parts[i].size(), 3000, (renderer));
  }
  return parts;
}

void MessageAcceptor::TraceMessageQueue(std::string_view renderer)
{
  if (m_queueTraceTimer.ElapsedSeconds() < 1.0)
    return;
  m_queueTraceTimer.Reset();

  for (auto const & record : FormatMessageQueueTrace(m_messageQueue.GetTrace(), renderer, ++m_queueTraceSample))
    LOG(LINFO, (record));
}
#endif

#ifdef DEBUG_MESSAGE_QUEUE

bool MessageAcceptor::IsQueueEmpty() const
{
  return m_messageQueue.IsEmpty();
}

size_t MessageAcceptor::GetQueueSize() const
{
  return m_messageQueue.GetSize();
}

#endif
}  // namespace df
