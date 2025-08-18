/// @author	Yury Melnichek mailto:melnichek@gmail.com
/// @date	07.01.2005
/// @brief	See http://gzip.rsdn.ru/forum/Message.aspx?mid=389127&only=1.

#pragma once

namespace base
{
namespace impl
{
/// Base class for all ScopeGuards.
class GuardBase
{
public:
  /// Disable ScopeGuard functionality on it's destruction
  void release() const { m_bDoRollback = false; }

protected:
  GuardBase() : m_bDoRollback(true) {}

  // Do something in the destructor
  mutable bool m_bDoRollback;
};

/// ScopeGuard for specific functor
template <typename TFunctor>
class GuardImpl : public GuardBase
{
public:
  explicit GuardImpl(TFunctor const & F) : m_Functor(F) {}

  ~GuardImpl()
  {
    if (m_bDoRollback)
      m_Functor();
  }

private:
  TFunctor m_Functor;
};
}  // namespace impl

typedef impl::GuardBase const & scope_guard;

/// Create scope_guard
template <typename TFunctor>
impl::GuardImpl<TFunctor> make_scope_guard(TFunctor const & F)
{
  return impl::GuardImpl<TFunctor>(F);
}
}  // namespace base

#define SCOPE_GUARD(name, func)                            \
  ::base::scope_guard name = base::make_scope_guard(func); \
  static_cast<void>(name)
