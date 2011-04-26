#include "../std/target_os.hpp"

#if !defined(OMIM_OS_BADA) && !defined(OMIM_OS_WINDOWS_NATIVE)

#include "condition.hpp"
#include "mutex.hpp"

#include <pthread.h>

namespace threads
{
  class Condition::Impl
  {
  public:
    Mutex m_Mutex;
    pthread_cond_t m_Condition;
  };
  
  Condition::Condition() : m_pImpl(new Condition::Impl)
  {
    ::pthread_cond_init(&m_pImpl->m_Condition, 0);
  }

  Condition::~Condition()
  {
    ::pthread_cond_destroy(&m_pImpl->m_Condition);
    delete m_pImpl;
  }

  void Condition::Signal()
  {
    ::pthread_cond_signal(&m_pImpl->m_Condition);
  }

  void Condition::Wait()
  {
    ::pthread_cond_wait(&m_pImpl->m_Condition, &m_pImpl->m_Mutex.m_Mutex);
  }

  void Condition::Lock()
  {
    m_pImpl->m_Mutex.Lock();
  }

  void Condition::Unlock()
  {
    m_pImpl->m_Mutex.Unlock();
  }
}

#endif
