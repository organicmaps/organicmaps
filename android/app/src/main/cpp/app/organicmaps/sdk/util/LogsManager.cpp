#include <jni.h>
#include "app/organicmaps/sdk/core/logging.hpp"

extern "C" {
JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_util_log_LogsManager_nativeToggleCoreDebugLogs(
    JNIEnv * /*env*/, jclass /*clazz*/, jboolean enabled)
{
  jni::ToggleDebugLogs(enabled);
}
}  // extern "C"
