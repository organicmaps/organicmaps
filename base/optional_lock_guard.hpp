#pragma once

#include <mutex>
#include <optional>

namespace base
{
template <typename Mutex>
class OptionalLockGuard
{
public:
  explicit OptionalLockGuard(std::optional<Mutex> & optionalMutex)
    : m_optionalGuard(optionalMutex ? std::optional<std::lock_guard<Mutex>>(*optionalMutex)
                                    : std::nullopt)
  {
  }

private:
  std::optional<std::lock_guard<Mutex>> m_optionalGuard;
};
}  // namespace base
