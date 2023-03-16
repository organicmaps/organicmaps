#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>

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

  // Updates the status.
  // Returns true iff current activity has been cancelled (either by the call
  // to Cancel or by timeout).
  virtual bool IsCancelled() const;

  // Updates the status of current activity and returns its value.
  virtual Status CancellationStatus() const;

private:
  // Checks whether |m_deadline| has exceeded. Must be called with |m_mutex| locked.
  void CheckDeadline() const;

  mutable std::mutex m_mutex;

  mutable Status m_status = Status::Active;

  std::optional<std::chrono::steady_clock::time_point> m_deadline;
};

std::string DebugPrint(Cancellable::Status status);
}  // namespace base
