#include "drape_frontend/threads_commutator.hpp"

#include "drape_frontend/message_acceptor.hpp"

#include "base/assert.hpp"

namespace df
{

void ThreadsCommutator::RegisterThread(ThreadName name, MessageAcceptor * acceptor)
{
  VERIFY(m_acceptors.insert(make_pair(name, acceptor)).second, ());
}

void ThreadsCommutator::PostMessage(ThreadName name, drape_ptr<Message> && message, MessagePriority priority)
{
  TAcceptorsMap::iterator it = m_acceptors.find(name);
  ASSERT(it != m_acceptors.end(), ());
  if (it != m_acceptors.end())
    it->second->PostMessage(move(message), priority);
}

} // namespace df

