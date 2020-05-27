#pragma once

#include "base/task_loop.hpp"

namespace platform
{
class GuiThread : public base::TaskLoop
{
public:
  TaskId Push(Task && task) override;
  TaskId Push(Task const & task) override;
};
}  // namespace platform
