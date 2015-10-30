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
