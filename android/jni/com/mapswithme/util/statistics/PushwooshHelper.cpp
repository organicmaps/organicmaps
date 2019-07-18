#include "com/mapswithme/core/jni_helper.hpp"

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

JNIEXPORT jstring JNICALL
Java_com_mapswithme_util_statistics_PushwooshHelper_nativeGetFormattedTimestamp(JNIEnv * env, jclass thiz)
{
  return jni::ToJavaString(env, GetPlatform().GetMarketingService().GetPushWooshTimestamp());
}
} // extern "C"
