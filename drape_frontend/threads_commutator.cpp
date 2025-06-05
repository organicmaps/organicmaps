#include "drape_frontend/threads_commutator.hpp"

#include "drape_frontend/base_renderer.hpp"

#include "base/assert.hpp"

#include <utility>

namespace df
{

void ThreadsCommutator::RegisterThread(ThreadName name, BaseRenderer * acceptor)
{
  VERIFY(m_acceptors.insert(std::make_pair(name, acceptor)).second, ());
}

void ThreadsCommutator::PostMessage(ThreadName name, drape_ptr<Message> && message, MessagePriority priority)
{
  TAcceptorsMap::iterator it = m_acceptors.find(name);
  ASSERT(it != m_acceptors.end(), ());
  if (it != m_acceptors.end() && it->second->CanReceiveMessages())
    it->second->PostMessage(std::move(message), priority);
}

}  // namespace df
