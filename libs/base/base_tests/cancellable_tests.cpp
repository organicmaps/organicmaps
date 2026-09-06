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
  constexpr auto kTimeout = chrono::milliseconds(20);

  promise<void> syncPromise;
  auto syncFuture = syncPromise.get_future();

  double x = 0.123;

  auto const fn = [&]
  {
    // Set the deadline once the thread runs, otherwise a slow thread start leaves too few iterations.
    cancellable.SetDeadline(chrono::steady_clock::now() + kTimeout);
    while (true)
    {
      if (cancellable.IsCancelled())
        break;

      x = cos(x);
    }

    syncPromise.set_value();
  };

  threads::SimpleThread thread(fn);
  syncFuture.wait();
  thread.join();

  TEST(cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::DeadlineExceeded, ());
  TEST(AlmostEqualAbs(x, 0.739, 1e-3), ());

  cancellable.Cancel();
  TEST(cancellable.IsCancelled(), ());
  TEST_EQUAL(cancellable.CancellationStatus(), Cancellable::Status::CancelCalled, ());
}
}  // namespace cancellable_tests
