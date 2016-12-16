#pragma once

#include <atomic>

namespace my
{
/// This class is a helper thread-safe class which can be mixed in
/// classes which represent some cancellable activities.
class Cancellable
{
public:
  Cancellable() : m_cancelled(false) {}

  virtual ~Cancellable() {}

  /// Marks current activity as not cancelled.
  virtual void Reset() { m_cancelled = false; }

  /// Marks current activity as cancelled.
  virtual void Cancel() { m_cancelled = true; }

  /// \return True is current activity is cancelled.
  virtual bool IsCancelled() const { return m_cancelled; }

private:
  std::atomic<bool> m_cancelled;
};
}  // namespace my
