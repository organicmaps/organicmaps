// Implementation of platform::GuiThread for Android.
// Delegates to the JNI UiThread class to post tasks on the main looper.
#include "platform/gui_thread.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include <memory>
#include <utility>

namespace platform
{
base::TaskLoop::PushResult GuiThread::Push(Task && task)
{
  JNIEnv * env = jni::GetEnv();
  // g_uiThreadClazz is cached in JNI_OnLoad (safe to use from any thread).
  static jmethodID const method = env->GetStaticMethodID(g_uiThreadClazz, "forwardToMainThread", "(J)V");

  auto t = new Task(std::move(task));
  env->CallStaticVoidMethod(g_uiThreadClazz, method, reinterpret_cast<jlong>(t));
  return {true, kNoId};
}

base::TaskLoop::PushResult GuiThread::Push(Task const & task)
{
  return Push(Task(task));
}
}  // namespace platform
