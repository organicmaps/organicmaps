#pragma once

#include <type_traits>

namespace base
{
// This enum is used to control the flow of ForEach invocations.
enum class ControlFlow
{
  Break,
  Continue
};

// A wrapper that calls |fn| with arguments |args|.
// To avoid excessive calls, |fn| may signal the end of execution via its return value,
// which should then be checked by the wrapper's user.
template <typename Fn>
class ControlFlowWrapper
{
public:
  template <typename Gn>
  explicit ControlFlowWrapper(Gn && gn) : m_fn(std::forward<Gn>(gn))
  {}

  template <typename... Args>
  std::enable_if_t<std::is_same_v<std::invoke_result_t<Fn, Args...>, ControlFlow>, ControlFlow> operator()(
      Args &&... args)
  {
    return m_fn(std::forward<Args>(args)...);
  }

  template <typename... Args>
  std::enable_if_t<std::is_same_v<std::invoke_result_t<Fn, Args...>, void>, ControlFlow> operator()(Args &&... args)
  {
    m_fn(std::forward<Args>(args)...);
    return ControlFlow::Continue;
  }

private:
  Fn m_fn;
};
}  // namespace base
