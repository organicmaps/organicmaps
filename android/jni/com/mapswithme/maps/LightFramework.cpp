#include "map/framework_light.hpp"
#include "map/framework_light_delegate.hpp"
#include "map/local_ads_manager.hpp"

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

jobject CreateFeatureId(JNIEnv * env, CampaignFeature const & data)
{
  static jmethodID const featureCtorId =
    jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  jni::TScopedLocalRef const countryId(env, jni::ToJavaString(env, data.m_countryId));
  return env->NewObject(g_featureIdClazz, featureCtorId, countryId.get(),
                        static_cast<jlong>(data.m_mwmVersion),
                        static_cast<jint>(data.m_featureIndex));
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

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetLocalAdsFeatures(JNIEnv * env, jclass clazz,
                                                                  jdouble lat, jdouble lon,
                                                                  jdouble radiusInMeters,
                                                                  jint maxCount)
{
  lightweight::Framework framework(lightweight::REQUEST_TYPE_LOCAL_ADS_FEATURES);
  auto const features = framework.GetLocalAdsFeatures(lat, lon, radiusInMeters, maxCount);

  static jclass const geoFenceFeatureClazz =
          jni::GetGlobalClassRef(env, "com/mapswithme/maps/geofence/GeoFenceFeature");
  // Java signature : GeoFenceFeature(FeatureId featureId,
  //                                  double latitude, double longitude)
  static jmethodID const geoFenceFeatureConstructor =
          jni::GetConstructorID(env, geoFenceFeatureClazz, "(Lcom/mapswithme/maps/bookmarks/data/FeatureId;DD)V");

  return jni::ToJavaArray(env, geoFenceFeatureClazz, features, [&](JNIEnv * jEnv,
                                                                   CampaignFeature const & data)
  {
      jni::TScopedLocalRef const featureId(env, CreateFeatureId(env, data));
      return env->NewObject(geoFenceFeatureClazz, geoFenceFeatureConstructor,
                            featureId.get(),
                            static_cast<jdouble>(data.m_lat),
                            static_cast<jdouble>(data.m_lon));
  });
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_LightFramework_nativeLogLocalAdsEvent(JNIEnv * env, jclass clazz,
                                                               jint type, jdouble lat, jdouble lon,
                                                               jint accuracyInMeters,
                                                               jlong mwmVersion, jstring countryId,
                                                               jint featureIndex)
{
  lightweight::Framework framework(lightweight::REQUEST_TYPE_LOCAL_ADS_STATISTICS);
  local_ads::Event event(static_cast<local_ads::EventType>(type), static_cast<long>(mwmVersion),
                         jni::ToNativeString(env, countryId), static_cast<uint32_t>(featureIndex),
                         static_cast<uint8_t>(1) /* zoom level */, local_ads::Clock::now(),
                         static_cast<double>(lat), static_cast<double>(lon),
                         static_cast<uint16_t>(accuracyInMeters));
  framework.GetLocalAdsStatistics()->RegisterEvent(std::move(event));
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
