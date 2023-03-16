#pragma once

#include "base/task_loop.hpp"

namespace platform
{
class GuiThread : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override;
  PushResult Push(Task const & task) override;
};
}  // namespace platform
