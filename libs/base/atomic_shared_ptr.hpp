#pragma once

#include "base/macros.hpp"

#include <atomic>
#include <memory>
#include <utility>

namespace base
{
// Template which provides methods for concurrently using shared pointers.
template <typename T>
class AtomicSharedPtr final
{
public:
  using ContentType = T const;
  using ValueType = std::shared_ptr<ContentType>;

  AtomicSharedPtr() : m_wrapped(std::make_shared<ContentType>()) {}

#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
  void Set(ValueType value) noexcept { m_wrapped.store(std::move(value)); }
  ValueType Get() const noexcept { return m_wrapped.load(); }

private:
  std::atomic<ValueType> m_wrapped;
#else
  // Some Xcode/Android NDK libc++ versions lack the atomic shared_ptr specialization.
  void Set(ValueType value) noexcept { std::atomic_store(&m_wrapped, std::move(value)); }
  ValueType Get() const noexcept { return std::atomic_load(&m_wrapped); }

private:
  ValueType m_wrapped;
#endif

  DISALLOW_COPY_AND_MOVE(AtomicSharedPtr);
};
}  // namespace base
