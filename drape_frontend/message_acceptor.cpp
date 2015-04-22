#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

namespace df
{

MessageAcceptor::MessageAcceptor()
  : m_infinityWaiting(false)
{
}

void MessageAcceptor::ProcessSingleMessage(unsigned maxTimeWait)
{
  m_infinityWaiting = (maxTimeWait == -1);
  drape_ptr<Message> message = m_messageQueue.PopMessage(maxTimeWait);
  m_infinityWaiting = false;

  if (message == nullptr)
    return;

  AcceptMessage(make_ref(message));
}

void MessageAcceptor::PostMessage(drape_ptr<Message> && message, MessagePriority priority)
{
  if (CanReceiveMessage())
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

bool MessageAcceptor::IsQueueEmpty()
{
  return m_messageQueue.IsEmpty();
}

} // namespace df
