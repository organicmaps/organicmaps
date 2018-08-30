#pragma once

#include "metrics/eye_info.hpp"

#include "base/atomic_shared_ptr.hpp"
#include "base/macros.hpp"

namespace eye
{
// Note This class IS thread-safe.
// All write operations are asynchronous and work on Platform::Thread::File thread.
// Read operations are synchronous and return shared pointer with constant copy of internal
// container.
class Eye
{
public:
  friend class EyeForTesting;
  using InfoType = base::AtomicSharedPtr<Info>::ValueType;

  class Event
  {
  public:
    static void TipShown(Tip::Type type, Tip::Event event);
  };

  static Eye & Instance();

  InfoType GetInfo() const;

private:
  Eye();

  void Save(InfoType const & info);

  // Event processing:
  void AppendTip(Tip::Type type, Tip::Event event);

  base::AtomicSharedPtr<Info> m_info;

  DISALLOW_COPY_AND_MOVE(Eye);
};
}  // namespace eye
