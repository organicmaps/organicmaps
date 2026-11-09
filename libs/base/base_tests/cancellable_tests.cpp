#include "testing/testing.hpp"

#include "base/cancellable.hpp"
#include "base/math.hpp"
#include "base/thread.hpp"

#include <atomic>
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

  auto const moved = std::move(copy);
  TEST(moved.IsCancelled(), ());
  TEST(copy.IsCancelled(), ("A moved-from token keeps the shared flag"));
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
  // A separate signal ends the wait, so a slow cancelling thread can't fail the test.
  atomic_bool cancelIssued{false};
  bool observed = false;
  threads::SimpleThread thread([token = cancellable, &cancelIssued, &observed]
  {
    while (!token.IsCancelled() && !cancelIssued.load(memory_order_acquire))
      this_thread::yield();
    observed = token.IsCancelled();
  });
  cancellable.Cancel();
  cancelIssued.store(true, memory_order_release);
  thread.join();
  TEST(observed, ());
}
}  // namespace cancellable_tests
