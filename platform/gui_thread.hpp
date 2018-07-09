#pragma once

#include "base/task_loop.hpp"

namespace platform
{
class GuiThread : public base::TaskLoop
{
public:
  bool Push(Task && task) override;
  bool Push(Task const & task) override;
};
}  // namespace platform
