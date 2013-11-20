#include "message_acceptor.hpp"

#include "message.hpp"

namespace df
{
  void MessageAcceptor::ProcessSingleMessage(bool waitMessage)
  {
    Message * message = m_messageQueue.PopMessage(waitMessage);
    if (message == NULL)
      return;

    AcceptMessage(message);
    delete message;
  }

  void MessageAcceptor::PostMessage(Message * message)
  {
    m_messageQueue.PushMessage(message);
  }

  void MessageAcceptor::CloseQueue()
  {
    m_messageQueue.CancelWait();
    ClearQueue();
  }

  void MessageAcceptor::ClearQueue()
  {
    Message * message;
    while ((message = m_messageQueue.PopMessage(false)) != NULL)
      delete message;
  }
}
