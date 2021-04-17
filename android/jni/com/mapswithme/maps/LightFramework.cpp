#include "map/framework_light.hpp"

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
}  // extern "C"
