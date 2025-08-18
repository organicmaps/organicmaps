#include "drape_frontend/drape_notifier.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "drape/drape_routine.hpp"

namespace df
{
DrapeNotifier::DrapeNotifier(ref_ptr<ThreadsCommutator> commutator)
  : m_commutator(commutator)
  , m_counter(kInvalidId + 1)
{}

uint64_t DrapeNotifier::Notify(ThreadsCommutator::ThreadName threadName,
                               base::DelayedThreadPool::Duration const & duration, bool repeating, Functor && functor)
{
  uint64_t const notifyId = m_counter++;
  NotifyImpl(threadName, duration, repeating, notifyId, std::move(functor));
  return notifyId;
}

void DrapeNotifier::NotifyImpl(ThreadsCommutator::ThreadName threadName,
                               base::DelayedThreadPool::Duration const & duration, bool repeating, uint64_t notifyId,
                               Functor && functor)
{
  dp::DrapeRoutine::RunDelayed(duration,
                               [this, threadName, duration, repeating, notifyId, functor = std::move(functor)]() mutable
  {
    m_commutator->PostMessage(threadName, make_unique_dp<NotifyRenderThreadMessage>(functor, notifyId),
                              MessagePriority::Normal);
    if (repeating)
      NotifyImpl(threadName, duration, repeating, notifyId, std::move(functor));
  });
}
}  // namespace df
