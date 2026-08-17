#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

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
