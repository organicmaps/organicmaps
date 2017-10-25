#include "com/mapswithme/platform/GuiThread.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

#include <memory>

namespace android
{
GuiThread::GuiThread(jobject processObject)
{
  JNIEnv * env = jni::GetEnv();

  m_object = env->NewGlobalRef(processObject);
  ASSERT(m_object, ());

  m_method = env->GetMethodID(env->GetObjectClass(m_object), "forwardToMainThread", "(J)V");
  ASSERT(m_method, ());
}

GuiThread::~GuiThread()
{
  JNIEnv * env = jni::GetEnv();
  env->DeleteGlobalRef(m_object);
}

// static
void GuiThread::ProcessTask(jlong task)
{
  std::unique_ptr<Task> t(reinterpret_cast<Task *>(task));
  (*t)();
}

bool GuiThread::Push(Task && task)
{
  // Pointer will be deleted in ProcessTask.
  auto t = new Task(std::move(task));
  jni::GetEnv()->CallVoidMethod(m_object, m_method, reinterpret_cast<jlong>(t));
  return true;
}

bool GuiThread::Push(Task const & task)
{
  // Pointer will be deleted in ProcessTask.
  auto t = new Task(task);
  jni::GetEnv()->CallVoidMethod(m_object, m_method, reinterpret_cast<jlong>(t));
  return true;
}
}  // namespace android
