#include "std/target_os.hpp"

#ifdef OMIM_OS_BADA

#include "base/condition.hpp"

#include <FBaseRtThreadMonitor.h>

namespace threads
{
  namespace impl
  {
    class ConditionImpl
    {
    public:
      Osp::Base::Runtime::Monitor m_Monitor;
    };
  }

  Condition::Condition() : m_pImpl(new impl::ConditionImpl())
  {   
    m_pImpl->m_Monitor.Construct();
  }

  Condition::~Condition()
  {
    delete m_pImpl;
  }

  void Condition::Signal(bool broadcast)
  {
    if (broadcast)
      m_pImpl->m_Monitor.NotifyAll();
    else
      m_pImpl->m_Monitor.Notify();
  }

  void Condition::Wait()
  {
    m_pImpl->m_Monitor.Wait();
  }

  void Condition::Lock()
  {
    m_pImpl->m_Monitor.Enter();
  }

  void Condition::Unlock()
  {
    m_pImpl->m_Monitor.Exit();
  }
}

#endif // OMIM_OS_BADA
