#pragma once

#include <functional>
#include <string>

namespace base
{
class TaskLoop
{
public:
  using Task = std::function<void()>;
  using TaskId = std::string;

  virtual ~TaskLoop() = default;

  virtual TaskId Push(Task && task) = 0;
  virtual TaskId Push(Task const & task) = 0;
};
}  // namespace base
