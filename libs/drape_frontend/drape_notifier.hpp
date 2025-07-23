#pragma once

#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"

#include "base/thread_pool_delayed.hpp"

#include <atomic>
#include <cstdint>
#include <functional>

namespace df
{
class DrapeNotifier
{
public:
  static uint64_t constexpr kInvalidId = 0;
  using Functor = std::function<void(uint64_t notifyId)>;

  explicit DrapeNotifier(ref_ptr<ThreadsCommutator> commutator);

  uint64_t Notify(ThreadsCommutator::ThreadName threadName, base::DelayedThreadPool::Duration const & duration,
                  bool repeating, Functor && functor);

private:
  void NotifyImpl(ThreadsCommutator::ThreadName threadName, base::DelayedThreadPool::Duration const & duration,
                  bool repeating, uint64_t notifyId, Functor && functor);

  ref_ptr<ThreadsCommutator> m_commutator;
  std::atomic<uint64_t> m_counter;
};
}  // namespace df
