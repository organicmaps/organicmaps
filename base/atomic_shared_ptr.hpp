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

  AtomicSharedPtr() = default;

  void Set(ValueType value) noexcept { atomic_store(&m_wrapped, value); }
  ValueType Get() const noexcept { return atomic_load(&m_wrapped); }

private:
  ValueType m_wrapped = std::make_shared<ContentType>();

  DISALLOW_COPY_AND_MOVE(AtomicSharedPtr);
};
}  // namespace base
