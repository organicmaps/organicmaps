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
  private:
    Condition & m_Condition;
  public:
    ConditionGuard(Condition & condition);
    ~ConditionGuard();
    void Wait();
    void Signal(bool broadcast = false);
  };
}
