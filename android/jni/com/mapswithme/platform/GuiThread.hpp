#pragma once

#include "base/task_loop.hpp"

#include <jni.h>

namespace android
{
class GuiThread : public base::TaskLoop
{
public:
  explicit GuiThread(jobject processObject);
  ~GuiThread() override;

  static void ProcessTask(jlong task);

  // TaskLoop overrides:
  TaskId Push(Task && task) override;
  TaskId Push(Task const & task) override;

private:
  jobject m_object = nullptr;
  jmethodID m_method = nullptr;
};
}  // namespace android
