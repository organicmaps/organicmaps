#include "map/framework_light.hpp"
#include "map/local_ads_manager.hpp"

#include "base/assert.hpp"

#include "com/mapswithme/core/jni_helper.hpp"

using namespace lightweight;

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_LightFramework_nativeIsAuthenticated(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_USER_AUTH_STATUS);
  return static_cast<jboolean>(framework.IsUserAuthenticated());
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetNumberUnsentUGC(JNIEnv * env, jclass clazz)
{
  Framework const framework(REQUEST_TYPE_NUMBER_OF_UNSENT_UGC);
  return static_cast<jint>(framework.GetNumberOfUnsentUGC());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_LightFramework_nativeGetLocalAdsFeatures(JNIEnv * env, jclass clazz,
                                                                  jdouble lat, jdouble lon,
                                                                  jdouble radiusInMeters,
                                                                  jint maxCount)
{
  Framework framework(REQUEST_TYPE_LOCAL_ADS_FEATURES);
  auto const features = framework.GetLocalAdsFeatures(lat, lon, radiusInMeters, maxCount);

  static jclass const geoFenceFeatureClazz =
          jni::GetGlobalClassRef(env, "com/mapswithme/maps/geofence/GeoFenceFeature");
  // Java signature : GeoFenceFeature(long mwmVersion, String countryId, int featureIndex,
  //                                  double latitude, double longitude)
  static jmethodID const geoFenceFeatureConstructor =
          jni::GetConstructorID(env, geoFenceFeatureClazz, "(JLjava/lang/String;IDD)V");

  return jni::ToJavaArray(env, geoFenceFeatureClazz, features, [&](JNIEnv * jEnv,
                                                                   CampaignFeature const & data)
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
Java_com_mapswithme_maps_LightFramework_nativeLogLocalAdsEvent(JNIEnv * env, jclass clazz,
                                                               jint type, jdouble lat, jdouble lon,
                                                               jint accuracyInMeters,
                                                               jlong mwmVersion, jstring countryId,
                                                               jint featureIndex)
{
  Framework framework(REQUEST_TYPE_LOCAL_ADS_STATISTICS);
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
  Framework framework(REQUEST_TYPE_NOTIFICATION);
  auto const notification = framework.GetNotification();

  if (!notification)
    return nullptr;

  // Type::UgcReview is only supported.
  CHECK_EQUAL(notification.get().m_type, notifications::NotificationCandidate::Type::UgcReview, ());

  static jclass const candidateId =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/background/NotificationCandidate");
  static jclass const mapObjectId =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/background/NotificationCandidate$MapObject");
  static jmethodID const candidateCtor = jni::GetConstructorID(
      env, candidateId, "(ILcom/mapswithme/maps/background/NotificationCandidate$MapObject;)V");
  static jmethodID const mapObjectCtor = jni::GetConstructorID(
      env, mapObjectId, "(DDLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  auto const & srcObject = notification.get().m_mapObject;
  ASSERT(srcObject, ());
  auto const readableName = jni::ToJavaString(env, srcObject->GetReadableName());
  auto const defaultName = jni::ToJavaString(env, srcObject->GetDefaultName());
  auto const type = jni::ToJavaString(env, srcObject->GetBestType());
  auto const mapObject = env->NewObject(mapObjectId, mapObjectCtor, srcObject->GetPos().x,
                                        srcObject->GetPos().y, readableName, defaultName, type);
  return env->NewObject(candidateId, candidateCtor, static_cast<jint>(notification.get().m_type),
                        mapObject);
}
}  // extern "C"
