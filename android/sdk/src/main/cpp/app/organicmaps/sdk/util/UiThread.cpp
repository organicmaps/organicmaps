#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/platform/GuiThread.hpp"

extern "C"
{
// static void nativeProcessTask(long taskPointer);
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_util_concurrency_UiThread_nativeProcessTask(JNIEnv * env, jclass clazz,
                                                                                            jlong taskPointer)
{
  android::GuiThread::ProcessTask(taskPointer);
}
}
