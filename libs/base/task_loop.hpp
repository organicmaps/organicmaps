#pragma once

#include <cstdint>
#include <functional>

namespace base
{
class TaskLoop
{
public:
  using Task = std::function<void()>;
  using TaskId = uint64_t;

  static TaskId constexpr kNoId = 0;

  struct PushResult
  {
    // Contains true when task is posted successfully.
    bool m_isSuccess = false;
    // Contains id of posted task which can be used to access the task to cancel/suspend/etc it.
    // kNoId is returned for tasks that cannot be accessed.
    TaskId m_id = kNoId;
  };

  virtual ~TaskLoop() = default;

  virtual PushResult Push(Task && task) = 0;
  virtual PushResult Push(Task const & task) = 0;
};
}  // namespace base
