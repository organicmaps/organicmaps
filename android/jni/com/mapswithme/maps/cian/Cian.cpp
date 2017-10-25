#include "com/mapswithme/maps/Framework.hpp"

#include "com/mapswithme/core/jni_helper.hpp"
#include "partners_api/cian_api.hpp"

#include "base/logging.cpp"

namespace
{
jclass g_cianClass;
jclass g_rentPlaceClass;
jclass g_rentOfferClass;
jmethodID g_rentPlaceConstructor;
jmethodID g_rentOfferConstructor;
jmethodID g_cianCallback;
jmethodID g_cianSuccessCallback;
jmethodID g_cianErrorCallback;
uint64_t g_requestId;
std::string g_id;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_cianClass)
    return;

  g_cianClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/cian/Cian");
  g_rentPlaceClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/cian/RentPlace");
  g_rentOfferClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/cian/RentOffer");

  g_rentPlaceConstructor =
      jni::GetConstructorID(env, g_rentPlaceClass,
                            "(DD[Lcom/mapswithme/maps/cian/RentOffer;)V");

  g_rentOfferConstructor =
      jni::GetConstructorID(env, g_rentOfferClass,
                            "(Ljava/lang/String;IDIILjava/lang/String;Ljava/lang/String;)V");

  g_cianSuccessCallback =
      jni::GetStaticMethodID(env, g_cianClass, "onRentPlacesReceived",
                             "([Lcom/mapswithme/maps/cian/RentPlace;Ljava/lang/String;)V");
  g_cianErrorCallback =
      jni::GetStaticMethodID(env, g_cianClass, "onErrorReceived",
                             "(I)V");
}

void OnRentPlacesReceived(std::vector<cian::RentPlace> const & places, uint64_t const requestId)
{
  if (g_requestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  auto const offerBuilder = [](JNIEnv * env, cian::RentOffer const & item)
  {
    jni::TScopedLocalRef jFlatType(env, jni::ToJavaString(env, item.m_flatType));
    jni::TScopedLocalRef jUrl(env, jni::ToJavaString(env, item.m_url));
    jni::TScopedLocalRef jAddress(env, jni::ToJavaString(env, item.m_address));
    return env->NewObject(g_rentOfferClass, g_rentOfferConstructor, jFlatType.get(),
                          item.m_roomsCount, item.m_priceRur, item.m_floorNumber,
                          item.m_floorsCount, jUrl.get(), jAddress.get());
  };

  auto const placeBuilder = [offerBuilder](JNIEnv * env, cian::RentPlace const & item)
  {
    return env->NewObject(g_rentPlaceClass, g_rentPlaceConstructor, item.m_latlon.lat,
                          item.m_latlon.lon,
                          jni::ToJavaArray(env, g_rentOfferClass, item.m_offers, offerBuilder));
  };

  jni::TScopedLocalObjectArrayRef jPlaces(env, jni::ToJavaArray(env, g_rentPlaceClass, places,
                                          placeBuilder));
  jni::TScopedLocalRef jId(env, jni::ToJavaString(env, g_id));

  env->CallStaticVoidMethod(g_cianClass, g_cianSuccessCallback, jPlaces.get(), jId.get());
}

void OnErrorReceived(int httpCode, uint64_t const requestId)
{
  if (g_requestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  env->CallStaticVoidMethod(g_cianClass, g_cianErrorCallback, httpCode);
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_com_mapswithme_maps_cian_Cian_nativeGetRentNearby(
    JNIEnv * env, jclass clazz, jobject policy, jdouble lat, jdouble lon, jstring id)
{
  PrepareClassRefs(env);

  g_id = jni::ToNativeString(env, id);
  ms::LatLon const pos(lat, lon);
  g_requestId = g_framework->GetRentNearby(env, policy, pos, &OnRentPlacesReceived,
                                                   &OnErrorReceived);
}
}  // extern "C"
