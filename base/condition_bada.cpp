#include "../std/target_os.hpp"

#ifdef OMIM_OS_BADA

#include "condition.hpp"

#include <FBaseRtThreadMonitor.h>

namespace threads
{
  class Condition::Impl
  {
  public:
    Osp::Base::Runtime::Monitor m_Monitor;
  };

  Condition::Condition() : m_pImpl(new Condition::Impl())
  {   
    m_pImpl->m_Monitor.Construct();
  }

  Condition::~Condition()
  {
    delete m_pImpl;
  }

  void Condition::Signal()
  {
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
