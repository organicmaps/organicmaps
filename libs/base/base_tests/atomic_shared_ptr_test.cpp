#include "testing/testing.hpp"

#include "base/atomic_shared_ptr.hpp"

#include <memory>

namespace
{
UNIT_TEST(AtomicSharedPtr_Snapshots)
{
  base::AtomicSharedPtr<int> value;
  auto const initial = value.Get();
  TEST(initial, ());
  TEST_EQUAL(*initial, 0, ());

  value.Set(std::make_shared<int>(42));
  auto const snapshot = value.Get();
  TEST_EQUAL(*snapshot, 42, ());
  TEST_EQUAL(*initial, 0, ());

  value.Set(nullptr);
  TEST(!value.Get(), ());
  TEST_EQUAL(*snapshot, 42, ());
}

}  // namespace
