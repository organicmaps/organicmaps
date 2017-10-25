#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "platform/platform.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_com_mapswithme_util_statistics_PushwooshHelper_nativeProcessFirstLaunch(JNIEnv * env, jclass thiz)
{
  GetPlatform().GetMarketingService().ProcessFirstLaunch();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_util_statistics_PushwooshHelper_nativeSendEditorAddObjectTag(JNIEnv * env, jclass thiz)
{
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kEditorAddDiscovered);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_util_statistics_PushwooshHelper_nativeSendEditorEditObjectTag(JNIEnv * env, jclass thiz)
{
  GetPlatform().GetMarketingService().SendPushWooshTag(marketing::kEditorEditDiscovered);
}
} // extern "C"
