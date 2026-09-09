#include "testing/testing.hpp"

#include "base/cancellable.hpp"
#include "base/math.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <cmath>
#include <future>

namespace cancellable_tests
{
using base::Cancellable;
using namespace std;

UNIT_TEST(Cancellable_Smoke)
{
  Cancellable cancellable;

  promise<void> syncPromise;
  auto syncFuture = syncPromise.get_future();

  double x = 0.123;

  auto const fn = [&]
  {
    for (size_t it = 0;; it++)
    {
      if (it > 100 && cancellable.IsCancelled())
        break;

      x = cos(x);
    }

    syncPromise.set_value();
  };

  threads::SimpleThread thread(fn);
  cancellable.Cancel();
  syncFuture.wait();
  thread.join();

  TEST(cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::CancelCalled, ());
  TEST(AlmostEqualAbs(x, 0.739, 1e-3), ());
}

UNIT_TEST(Cancellable_Deadline)
{
  Cancellable cancellable;
  auto const now = chrono::steady_clock::now();
  cancellable.SetDeadline(now + chrono::hours(1));
  TEST(!cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::Active, ());

  cancellable.SetDeadline(now - chrono::hours(1));
  TEST(cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::DeadlineExceeded, ());

  cancellable.Cancel();
  TEST(cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::CancelCalled, ());
}
}  // namespace cancellable_tests
