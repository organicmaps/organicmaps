#pragma once

#include "base/macros.hpp"

#include <memory>

namespace base
{
// Template which provides methods for concurrently using shared pointers.
template <typename T>
class AtomicSharedPtr
{
public:
  AtomicSharedPtr() = default;

  void Set(std::shared_ptr<T const> value) noexcept { atomic_store(&m_wrapped, value); }
  std::shared_ptr<T const> Get() const noexcept { return atomic_load(&m_wrapped); }

private:
  std::shared_ptr<T const> m_wrapped = std::make_shared<T const>();
};
}  // namespace base
