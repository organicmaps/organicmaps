#include "drape_frontend/threads_commutator.hpp"

#include "drape_frontend/base_renderer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace df
{

void ThreadsCommutator::RegisterThread(ThreadName name, BaseRenderer * acceptor)
{
  VERIFY(m_acceptors.emplace(name, acceptor).second, ());
}

void ThreadsCommutator::PostMessage(ThreadName name, drape_ptr<Message> && message, MessagePriority priority)
{
  auto it = m_acceptors.find(name);
  ASSERT(it != m_acceptors.end(), ());
  if (it->second->CanReceiveMessages())
    it->second->PostMessage(std::move(message), priority);
  else
    LOG(LDEBUG, ("!VNG! Can't receive messages:", name));
}

} // namespace df
