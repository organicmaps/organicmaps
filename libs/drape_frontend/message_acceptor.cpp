#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

#ifdef DRAPE_QUEUE_TRACE
#include "base/logging.hpp"

#include <sstream>
#endif

namespace df
{
bool MessageAcceptor::ProcessSingleMessage(bool waitForMessage)
{
  drape_ptr<Message> message = m_messageQueue.PopMessage(waitForMessage);
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
void MessageAcceptor::TraceMessageQueue()
{
  if (m_queueTraceTimer.ElapsedSeconds() < 1.0)
    return;
  m_queueTraceTimer.Reset();

  auto const trace = m_messageQueue.GetTrace();
  std::ostringstream out;
  out << "DrapeQueue {\"size\":" << trace.m_size << ",\"peak\":" << trace.m_peak << ",\"types\":{";
  bool first = true;
  for (auto const & [type, counts] : trace.m_types)
  {
    if (!first)
      out << ',';
    first = false;
    out << '"' << DebugPrint(type) << "\":{\"size\":" << counts.m_size << ",\"peak\":" << counts.m_peak
        << ",\"enqueued\":" << counts.m_enqueued << ",\"popped\":" << counts.m_popped
        << ",\"filtered\":" << counts.m_filtered << ",\"rejected\":" << counts.m_rejected
        << ",\"cleared\":" << counts.m_cleared << '}';
  }
  out << "}}";
  LOG(LINFO, (out.str()));
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
