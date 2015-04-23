#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

namespace df
{

void MessageAcceptor::ProcessSingleMessage(unsigned maxTimeWait)
{
  drape_ptr<Message> message = m_messageQueue.PopMessage(maxTimeWait);
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

} // namespace df
