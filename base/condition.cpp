#include "condition.hpp"

#include "std/target_os.hpp"

#if defined(OMIM_OS_WINDOWS_NATIVE)
  #include "condition_windows_native.cpp"
#else
  #include "condition_posix.cpp"
#endif

namespace threads
{
  ConditionGuard::ConditionGuard(Condition & condition)
    : m_Condition(condition)
  {
    m_Condition.Lock();
  }

  ConditionGuard::~ConditionGuard()
  {
    m_Condition.Unlock();
  }

  void ConditionGuard::Wait(unsigned ms)
  {
    m_Condition.Wait(ms);
  }

  void ConditionGuard::Signal(bool broadcast)
  {
    m_Condition.Signal(broadcast);
  }
}

