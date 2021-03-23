#include "map/framework_light.hpp"
#include "map/framework_light_delegate.hpp"

#include "base/assert.hpp"

#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_LightFramework_nativeIsAuthenticated(JNIEnv * env, jclass clazz)
{
  lightweight::Framework const framework(lightweight::REQUEST_TYPE_USER_AUTH_STATUS);
  return static_cast<jboolean>(framework.IsUserAuthenticated());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetNumberUnsentUGC(JNIEnv * env, jclass clazz)
{
  lightweight::Framework const framework(lightweight::REQUEST_TYPE_NUMBER_OF_UNSENT_UGC);
  return static_cast<jint>(framework.GetNumberOfUnsentUGC());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_LightFramework_nativeMakeFeatureId(JNIEnv * env, jclass clazz,
                                                            jstring mwmName, jlong mwmVersion,
                                                            jint featureIndex)
{
  auto const featureId = lightweight::FeatureParamsToString(
    static_cast<int64_t>(mwmVersion), jni::ToNativeString(env, mwmName),
    static_cast<uint32_t>(featureIndex));

  return jni::ToJavaString(env, featureId);
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetNotification(JNIEnv * env, jclass clazz)
{
  lightweight::Framework framework(lightweight::REQUEST_TYPE_NOTIFICATION);
  if (g_framework)
    framework.SetDelegate(std::make_unique<FrameworkLightDelegate>(*g_framework->NativeFramework()));
  auto const notification = framework.GetNotification();

  if (!notification)
    return nullptr;

  auto const & n = *notification;
  // Type::UgcReview is only supported.
  CHECK_EQUAL(n.GetType(), notifications::NotificationCandidate::Type::UgcReview, ());

  static jclass const candidateId =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/background/NotificationCandidate$UgcReview");
  static jmethodID const candidateCtor = jni::GetConstructorID(
      env, candidateId,
      "(DDLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  auto const readableName = jni::ToJavaString(env, n.GetReadableName());
  auto const defaultName = jni::ToJavaString(env, n.GetDefaultName());
  auto const type = jni::ToJavaString(env, n.GetBestFeatureType());
  auto const address = jni::ToJavaString(env, n.GetAddress());
  return env->NewObject(candidateId, candidateCtor, n.GetPos().x, n.GetPos().y, readableName,
                        defaultName, type, address);
}
}  // extern "C"
