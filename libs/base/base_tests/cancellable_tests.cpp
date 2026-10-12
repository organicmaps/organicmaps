#include "testing/testing.hpp"

#include "base/cancellable.hpp"
#include "base/math.hpp"
#include "base/thread.hpp"

#include <chrono>
#include <cmath>
#include <future>
#include <thread>

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

UNIT_TEST(SharedCancellable_CopiesShareCancellation)
{
  base::SharedCancellable cancellable;
  auto copy = cancellable;
  base::SharedCancellable const independent;
  TEST(!cancellable.IsCancelled(), ());
  TEST(!copy.IsCancelled(), ());

  copy.Cancel();
  TEST(cancellable.IsCancelled(), ());
  TEST(copy.IsCancelled(), ());
  TEST(!independent.IsCancelled(), ());
}

UNIT_TEST(SharedCancellable_CopyOutlivesOwner)
{
  base::SharedCancellable observer;
  {
    base::SharedCancellable owner;
    observer = owner;
    owner.Cancel();
  }
  TEST(observer.IsCancelled(), ());
}

UNIT_TEST(SharedCancellable_CancelAcrossThreads)
{
  base::SharedCancellable cancellable;
  promise<void> started;
  auto startedFuture = started.get_future();
  promise<bool> observed;
  auto observedFuture = observed.get_future();
  threads::SimpleThread thread([token = cancellable, &started, &observed]
  {
    auto const deadline = chrono::steady_clock::now() + chrono::seconds(5);
    started.set_value();
    while (!token.IsCancelled() && chrono::steady_clock::now() < deadline)
      this_thread::yield();
    observed.set_value(token.IsCancelled());
  });
  startedFuture.wait();
  cancellable.Cancel();
  thread.join();
  TEST(observedFuture.get(), ());
}
}  // namespace cancellable_tests
