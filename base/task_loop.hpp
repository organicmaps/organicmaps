#pragma once

#include <functional>

namespace base
{
class TaskLoop
{
public:
  using Task = std::function<void()>;

  virtual ~TaskLoop() = default;

  virtual bool Push(Task && task) = 0;
  virtual bool Push(Task const & task) = 0;
};
}  // namespace base
