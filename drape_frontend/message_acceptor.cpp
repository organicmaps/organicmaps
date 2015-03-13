#include "drape_frontend/message_acceptor.hpp"

#include "drape_frontend/message.hpp"

namespace df
{

void MessageAcceptor::ProcessSingleMessage(unsigned maxTimeWait)
{
  dp::TransferPointer<Message> transferMessage = m_messageQueue.PopMessage(maxTimeWait);
  dp::MasterPointer<Message> message(transferMessage);
  if (message.IsNull())
    return;

  AcceptMessage(message.GetRefPointer());
  message.Destroy();
}

void MessageAcceptor::PostMessage(dp::TransferPointer<Message> message, MessagePriority priority)
{
  m_messageQueue.PushMessage(message, priority);
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
