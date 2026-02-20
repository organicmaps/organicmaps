#pragma once

#include "base/macros.hpp"

#include <memory>

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

  void Set(ValueType value) noexcept { std::atomic_store(&m_wrapped, std::move(value)); }
  ValueType Get() const noexcept { return std::atomic_load(&m_wrapped); }

private:
  ValueType m_wrapped;

  DISALLOW_COPY_AND_MOVE(AtomicSharedPtr);
};
}  // namespace base
