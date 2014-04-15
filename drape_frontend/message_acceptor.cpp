#include "message_acceptor.hpp"

#include "message.hpp"

namespace df
{
  void MessageAcceptor::ProcessSingleMessage(unsigned maxTimeWait)
  {
    TransferPointer<Message> transferMessage = m_messageQueue.PopMessage(maxTimeWait);
    MasterPointer<Message> message(transferMessage);
    if (message.IsNull())
      return;

    AcceptMessage(message.GetRefPointer());
    message.Destroy();
  }

  void MessageAcceptor::PostMessage(TransferPointer<Message> message)
  {
    m_messageQueue.PushMessage(message);
  }

  void MessageAcceptor::CloseQueue()
  {
    m_messageQueue.CancelWait();
    m_messageQueue.ClearQuery();
  }
}
