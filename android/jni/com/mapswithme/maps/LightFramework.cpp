#include "map/framework_light.hpp"
#include "map/local_ads_manager.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

using namespace lightweight;

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_LightFramework_nativeIsAuthenticated(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_USER_AUTH_STATUS);
  return static_cast<jboolean>(framework.Get<REQUEST_TYPE_USER_AUTH_STATUS>());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetNumberUnsentUGC(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_NUMBER_OF_UNSENT_UGC);
  return static_cast<jint>(framework.Get<REQUEST_TYPE_NUMBER_OF_UNSENT_UGC>());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetLocalAdsFeatures(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon,
                                                                  jdouble radiusInMeters, jint maxCount)
{
  Framework framework(REQUEST_TYPE_LOCAL_ADS_FEATURES);
  auto const features = framework.Get<REQUEST_TYPE_LOCAL_ADS_FEATURES>(lat, lon, radiusInMeters, maxCount);

  static jclass const geoFenceFeatureClazz = jni::GetGlobalClassRef(env,
                                                                    "com/mapswithme/maps/api/GeoFenceFeature");
  // Java signature : GeoFenceFeature(long mwmVersion, String countryId, int featureIndex,
  //                                  double latitude, double longitude)
  static jmethodID const geoFenceFeatureConstructor =
          jni::GetConstructorID(env, geoFenceFeatureClazz, "(JLjava/lang/String;IDD)V");

  return jni::ToJavaArray(env, geoFenceFeatureClazz, features, [&](JNIEnv * jEnv, CampaignFeature const & data)
  {
      jni::TScopedLocalRef const countryId(env, jni::ToJavaString(env, data.m_countryId));
      return env->NewObject(geoFenceFeatureClazz, geoFenceFeatureConstructor,
                            static_cast<jlong>(data.m_mwmVersion),
                            countryId.get(),
                            static_cast<jint>(data.m_featureIndex),
                            static_cast<jdouble>(data.m_lat),
                            static_cast<jdouble>(data.m_lon));
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_LightFramework_nativeLogLocalAdsEvent(JNIEnv * env, jclass clazz, jint type,
                                                               jdouble lat, jdouble lon, jint accuracyInMeters,
                                                               jlong mwmVersion, jstring countryId, jint featureIndex)
{
  Framework framework(REQUEST_TYPE_LOCAL_ADS_STATISTICS);
  local_ads::Event event(static_cast<local_ads::EventType>(type), static_cast<long>(mwmVersion),
                         jni::ToNativeString(env, countryId), static_cast<uint32_t>(featureIndex),
                         static_cast<uint8_t>(1) /* zoom level*/, local_ads::Clock::now(),
                         static_cast<double>(lat), static_cast<double>(lon),
                         static_cast<uint16_t>(accuracyInMeters));
  framework.Get<REQUEST_TYPE_LOCAL_ADS_STATISTICS>()->RegisterEvent(std::move(event));
}
}  // extern "C"
