#include "../std/target_os.hpp"

#ifdef OMIM_OS_WINDOWS_NATIVE

#include "condition.hpp"
#include "mutex.hpp"

#include "../std/windows.hpp"

namespace threads
{
  /// Implements mutexed condition semantics
  class Condition::Impl
  {
  public:
    CONDITION_VARIABLE m_Condition;
    Mutex m_mutex;
  };
  
  Condition::Condition() : m_pImpl(new Condition::Impl())
  {
    ::InitializeConditionVariable(&m_pImpl->m_Condition);
  }

  Condition::~Condition()
  {
    delete m_pImpl;
  }

  void Condition::Signal()
  {
    ::WakeConditionVariable(&m_pImpl->m_Condition);
  }

  void Condition::Wait()
  {
    ::SleepConditionVariableCS(&m_pImpl->m_Condition, &m_pImpl->m_mutex.m_Mutex, INFINITE);
  }

  void Condition::Lock()
  {
    m_pImpl->m_mutex.Lock();
  }

  void Condition::Unlock()
  {
    m_pImpl->m_mutex.Unlock();
  }
}

#endif // OMIM_OS_WINDOWS_NATIVE
