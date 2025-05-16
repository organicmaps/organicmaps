#include "app/organicmaps/platform/GuiThread.hpp"

#include "app/organicmaps/core/jni_helper.hpp"

#include <memory>

namespace android
{
GuiThread::GuiThread()
{
  JNIEnv * env = jni::GetEnv();

  m_class = jni::GetGlobalClassRef(env, "app/organicmaps/util/concurrency/UiThread");
  ASSERT(m_class, ());

  m_method = env->GetStaticMethodID(m_class, "forwardToMainThread", "(J)V");
  ASSERT(m_method, ());
}

GuiThread::~GuiThread()
{
  JNIEnv * env = jni::GetEnv();
  env->DeleteGlobalRef(m_class);
}

// static
void GuiThread::ProcessTask(jlong task)
{
  std::unique_ptr<Task> t(reinterpret_cast<Task *>(task));
  (*t)();
}

base::TaskLoop::PushResult GuiThread::Push(Task && task)
{
  // Pointer will be deleted in ProcessTask.
  auto t = new Task(std::move(task));
  jni::GetEnv()->CallStaticVoidMethod(m_class, m_method, reinterpret_cast<jlong>(t));
  return {true, kNoId};
}

base::TaskLoop::PushResult GuiThread::Push(Task const & task)
{
  // Pointer will be deleted in ProcessTask.
  auto t = new Task(task);
  jni::GetEnv()->CallStaticVoidMethod(m_class, m_method, reinterpret_cast<jlong>(t));
  return {true, kNoId};
}
}  // namespace android
