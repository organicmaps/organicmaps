#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

namespace df
{

MessageAcceptor::MessageAcceptor()
  : m_infinityWaiting(false)
{
}

bool MessageAcceptor::ProcessSingleMessage(bool waitForMessage)
{
  m_infinityWaiting = waitForMessage;
  drape_ptr<Message> message = m_messageQueue.PopMessage(waitForMessage);
  m_infinityWaiting = false;

  if (message == nullptr)
    return false;

  AcceptMessage(make_ref(message));
  return true;
}

void MessageAcceptor::EnableMessageFiltering(TFilterMessageFn needFilterMessageFn)
{
  m_needFilterMessageFn = needFilterMessageFn;
  m_messageQueue.FilterMessages(needFilterMessageFn);
}

void MessageAcceptor::DisableMessageFiltering()
{
  m_needFilterMessageFn = nullptr;
}

void MessageAcceptor::PostMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  if (m_needFilterMessageFn != nullptr && m_needFilterMessageFn(make_ref(message)))
    return;

  m_messageQueue.PushMessage(move(message), priority);
}

void MessageAcceptor::CloseQueue()
{
  m_messageQueue.CancelWait();
  m_messageQueue.ClearQuery();
}

void MessageAcceptor::CancelMessageWaiting()
{
  m_messageQueue.CancelWait();
}

bool MessageAcceptor::IsInInfinityWaiting() const
{
  return m_infinityWaiting;
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

} // namespace df
