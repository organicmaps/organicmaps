#include "base/cancellable.hpp"

#include "base/assert.hpp"

namespace base
{
void Cancellable::Reset()
{
  std::lock_guard lock(m_mutex);

  m_status = Status::Active;
  m_deadline = {};
}

void Cancellable::Cancel()
{
  std::lock_guard lock(m_mutex);

  m_status = Status::CancelCalled;
}

void Cancellable::SetDeadline(std::chrono::steady_clock::time_point const & deadline)
{
  std::lock_guard lock(m_mutex);

  m_deadline = deadline;
  CheckDeadline();
}

bool Cancellable::IsCancelled() const
{
  return CancellationStatus() != Status::Active;
}

Cancellable::Status Cancellable::CancellationStatus() const
{
  std::lock_guard lock(m_mutex);

  CheckDeadline();
  return m_status;
}

void Cancellable::CheckDeadline() const
{
  if (m_status == Status::Active && m_deadline && *m_deadline < std::chrono::steady_clock::now())
    m_status = Status::DeadlineExceeded;
}

std::string DebugPrint(Cancellable::Status status)
{
  switch (status)
  {
  case Cancellable::Status::Active: return "Active";
  case Cancellable::Status::CancelCalled: return "CancelCalled";
  case Cancellable::Status::DeadlineExceeded: return "DeadlineExceeded";
  }
  UNREACHABLE();
}
}  // namespace base
