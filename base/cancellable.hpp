#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

#include <boost/optional.hpp>

namespace base
{
// This is a helper thread-safe class which can be mixed in
// classes which represent some cancellable activities.
class Cancellable
{
public:
  enum class Status
  {
    Active,
    CancelCalled,
    DeadlineExceeded,
  };

  Cancellable() = default;

  virtual ~Cancellable() {}

  // Marks current activity as not cancelled.
  // Resets the deadline if it was present.
  virtual void Reset();

  // Marks current activity as cancelled.
  virtual void Cancel();

  // Sets a deadline after which the activity is cancelled.
  virtual void SetDeadline(std::chrono::steady_clock::time_point const & t);

  // Returns true iff current activity has been cancelled (either by the call
  // to Cancel or by timeout).
  virtual bool IsCancelled() const;

  // Returns the latest known status. Does not update the status before returning,
  // i.e. may miss crossing the deadline if the latest update was too long ago.
  virtual Status CancellationStatus() const;

private:
  // Checks whether |m_deadline| has exceeded. Must be called with |m_mutex| locked.
  void CheckDeadline() const;

  mutable std::mutex m_mutex;

  mutable Status m_status = Status::Active;

  boost::optional<std::chrono::steady_clock::time_point> m_deadline;
};

std::string DebugPrint(Cancellable::Status status);
}  // namespace base
