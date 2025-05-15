#pragma once

#include "base/task_loop.hpp"

#include <jni.h>

namespace android
{
class GuiThread : public base::TaskLoop
{
public:
  GuiThread();
  ~GuiThread() override;

  static void ProcessTask(jlong task);

  // TaskLoop overrides:
  PushResult Push(Task && task) override;
  PushResult Push(Task const & task) override;

private:
  jclass m_class = nullptr;
  jmethodID m_method = nullptr;
};
}  // namespace android
