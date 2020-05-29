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

  static TaskId constexpr kIncorrectId = 0;

  virtual ~TaskLoop() = default;

  virtual TaskId Push(Task && task) = 0;
  virtual TaskId Push(Task const & task) = 0;
};
}  // namespace base
