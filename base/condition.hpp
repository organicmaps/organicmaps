#pragma once

namespace threads
{
  namespace impl
  {
    class ConditionImpl;
  }

  /// Implements mutexed condition semantics
  class Condition
  {
    impl::ConditionImpl * m_pImpl;

    Condition(Condition const &);
    Condition & operator=(Condition const &);

  public:        
    Condition();
    ~Condition();

    void Signal(bool broadcast = false);
    void Wait();
    void Lock();
    void Unlock();
  };

  /// ScopeGuard wrapper around mutex
  class ConditionGuard
  {
  public:
    ConditionGuard(Condition & condition)
      : m_Condition(condition) { m_Condition.Lock(); }
    ~ConditionGuard() { m_Condition.Unlock(); }
    void Wait() { m_Condition.Wait(); }
    void Signal(bool broadcast = false) { m_Condition.Signal(broadcast); }
  private:
    Condition & m_Condition;
  };
}
