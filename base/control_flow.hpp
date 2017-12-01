#pragma once

#include <utility>

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
struct ControlFlowWrapper
{
  template <typename Gn>
  explicit ControlFlowWrapper(Gn && gn) : m_fn(std::forward<Gn>(gn))
  {
  }

  template <typename... Args>
  typename std::enable_if<
      std::is_same<typename std::result_of<Fn(Args...)>::type, base::ControlFlow>::value,
      base::ControlFlow>::type
  operator()(Args &&... args)
  {
    return m_fn(std::forward<Args>(args)...);
  }

  template <typename... Args>
  typename std::enable_if<std::is_same<typename std::result_of<Fn(Args...)>::type, void>::value,
                          base::ControlFlow>::type
  operator()(Args &&... args)
  {
    m_fn(std::forward<Args>(args)...);
    return ControlFlow::Continue;
  }

  Fn m_fn;
};
}  // namespace base
