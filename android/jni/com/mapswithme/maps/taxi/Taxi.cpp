#include "../Framework.hpp"

#include "../../core/jni_helper.hpp"
#include "partners_api/taxi_provider.hpp"

namespace
{
jclass g_taxiClass;
jclass g_productClass;
jclass g_routingControllerClass;
jclass g_taxiInfoClass;
jmethodID g_taxiInfoConstructor;
jobject g_routingControllerInstance;
jmethodID g_productConstructor;
jmethodID g_routingControllerGetMethod;
jmethodID g_taxiInfoCallbackMethod;
jmethodID g_taxiErrorCallbackMethod;
jclass g_taxiLinksClass;
jmethodID g_taxiLinksConstructor;
uint64_t g_lastRequestId;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_taxiClass)
    return;

  g_taxiClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfo");
  g_productClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfo$Product");
  g_routingControllerClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/routing/RoutingController");
  g_productConstructor = jni::GetConstructorID(
      env, g_productClass,
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  g_taxiInfoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfo");
  g_routingControllerGetMethod = jni::GetStaticMethodID(
      env, g_routingControllerClass, "get", "()Lcom/mapswithme/maps/routing/RoutingController;");
  g_routingControllerInstance =
      env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);
  g_taxiInfoCallbackMethod =
      jni::GetMethodID(env, g_routingControllerInstance, "onTaxiInfoReceived",
                       "(Lcom/mapswithme/maps/taxi/TaxiInfo;)V");
  g_taxiErrorCallbackMethod = jni::GetMethodID(env, g_routingControllerInstance,
                                               "onTaxiError", "(Ljava/lang/String;)V");
  g_taxiInfoConstructor = jni::GetConstructorID(env, g_taxiInfoClass,
                                                "([Lcom/mapswithme/maps/taxi/TaxiInfo$Product;)V");
  g_taxiLinksClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiLinks");
  g_taxiLinksConstructor =
      jni::GetConstructorID(env, g_taxiLinksClass, "(Ljava/lang/String;Ljava/lang/String;)V");
}

void OnTaxiInfoReceived(taxi::ProvidersContainer const & products, uint64_t const requestId)
{
  GetPlatform().RunOnGuiThread([=]() {
    if (g_lastRequestId != requestId)
      return;
    // TODO Dummy, must be changed by android developer.
    /*
        CHECK(!products.empty(), ("List of the products cannot be empty"));

        JNIEnv * env = jni::GetEnv();

        auto const uberProducts = jni::ToJavaArray(
            env, g_productClass, products[0].GetProducts(),
            [](JNIEnv * env, taxi::Product const & item) {
              return env->NewObject(
                  g_productClass, g_productConstructor, jni::ToJavaString(env, item.m_productId),
                  jni::ToJavaString(env, item.m_name), jni::ToJavaString(env, item.m_time),
                  jni::ToJavaString(env, item.m_price));
            });
        jobject const routingControllerInstance =
            env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);
        env->CallVoidMethod(routingControllerInstance, g_uberInfoCallbackMethod,
                            env->NewObject(g_uberInfoClass, g_uberInfoConstructor, uberProducts));
    */
  });
}

void OnTaxiError(taxi::ErrorsContainer const & errors, uint64_t const requestId)
{
  GetPlatform().RunOnGuiThread([=]() {
    if (g_lastRequestId != requestId)
      return;
    // TODO Dummy, must be changed by android developer.
    /*
        JNIEnv * env = jni::GetEnv();

        static jclass const errCodeClass =
       env->FindClass("com/mapswithme/maps/uber/Uber$ErrorCode"); ASSERT(errCodeClass, ());

        jobject const routingControllerInstance =
            env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);

        env->CallVoidMethod(routingControllerInstance, g_uberErrorCallbackMethod,
                            jni::ToJavaString(env, taxi::DebugPrint(code)));
    */
  });
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_com_mapswithme_maps_taxi_Taxi_nativeRequestTaxiProducts(
    JNIEnv * env, jclass clazz, jobject policy, jdouble srcLat, jdouble srcLon, jdouble dstLat,
    jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  g_lastRequestId =
      g_framework->RequestTaxiProducts(env, policy, from, to, &OnTaxiInfoReceived, &OnTaxiError);
}

JNIEXPORT jobject JNICALL Java_com_mapswithme_maps_taxi_Taxi_nativeGetUberLinks(
    JNIEnv * env, jclass clazz, jobject policy, jstring productId, jdouble srcLat, jdouble srcLon,
    jdouble dstLat, jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  taxi::RideRequestLinks const links =
      g_framework->GetTaxiLinks(env, policy, jni::ToNativeString(env, productId), from, to);
  return env->NewObject(g_taxiLinksClass, g_taxiLinksConstructor,
                        jni::ToJavaString(env, links.m_deepLink),
                        jni::ToJavaString(env, links.m_universalLink));
}
}  // extern "C"
