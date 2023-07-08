#include "testing/testing.hpp"

#include "base/optional_lock_guard.hpp"

#include <mutex>
#include <optional>

namespace
{
using namespace base;

UNIT_TEST(OptionalLockGuard_Smoke)
{
  std::optional<std::mutex> optMtx;
  {
    base::OptionalLockGuard guard(optMtx);
  }

  std::optional<std::mutex> empty = std::nullopt;
  base::OptionalLockGuard<std::mutex> guard(empty);
}
}  // namespace
