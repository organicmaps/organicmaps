#include "../Framework.hpp"

#include "../../core/jni_helper.hpp"
#include "partners_api/uber_api.hpp"

namespace
{
jclass g_uberClass;
jclass g_productClass;
jclass g_routingControllerClass;
jclass g_uberInfoClass;
jmethodID g_uberInfoConstructor;
jobject g_routingControllerInstance;
jmethodID g_productConstructor;
jmethodID g_routingControllerGetMethod;
jmethodID g_uberInfoCallbackMethod;
jmethodID g_uberErrorCallbackMethod;
jclass g_uberLinksClass;
jmethodID g_uberLinksConstructor;
uint64_t g_lastRequestId;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_uberClass)
    return;

  g_uberClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/uber/UberInfo");
  g_productClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/uber/UberInfo$Product");
  g_routingControllerClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/routing/RoutingController");
  g_productConstructor = jni::GetConstructorID(
      env, g_productClass,
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  g_uberInfoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/uber/UberInfo");
  g_routingControllerGetMethod = jni::GetStaticMethodID(
      env, g_routingControllerClass, "get", "()Lcom/mapswithme/maps/routing/RoutingController;");
  g_routingControllerInstance =
      env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);
  g_uberInfoCallbackMethod =
      jni::GetMethodID(env, g_routingControllerInstance, "onUberInfoReceived",
                       "(Lcom/mapswithme/maps/uber/UberInfo;)V");
  g_uberErrorCallbackMethod = jni::GetMethodID(env, g_routingControllerInstance,
                                               "onUberError", "(Ljava/lang/String;)V");
  g_uberInfoConstructor = jni::GetConstructorID(env, g_uberInfoClass,
                                                "([Lcom/mapswithme/maps/uber/UberInfo$Product;)V");
  g_uberLinksClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/uber/UberLinks");
  g_uberLinksConstructor =
      jni::GetConstructorID(env, g_uberLinksClass, "(Ljava/lang/String;Ljava/lang/String;)V");
}

void OnUberInfoReceived(vector<uber::Product> const & products, uint64_t const requestId)
{
  GetPlatform().RunOnGuiThread([=]() {
    if (g_lastRequestId != requestId)
      return;

    CHECK(!products.empty(), ("List of the products cannot be empty"));

    JNIEnv * env = jni::GetEnv();

    auto const uberProducts = jni::ToJavaArray(
        env, g_productClass, products, [](JNIEnv * env, uber::Product const & item) {
          return env->NewObject(
              g_productClass, g_productConstructor, jni::ToJavaString(env, item.m_productId),
              jni::ToJavaString(env, item.m_name), jni::ToJavaString(env, item.m_time),
              jni::ToJavaString(env, item.m_price));
        });
    jobject const routingControllerInstance =
        env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);
    env->CallVoidMethod(routingControllerInstance, g_uberInfoCallbackMethod,
                        env->NewObject(g_uberInfoClass, g_uberInfoConstructor, uberProducts));
  });
}

void OnUberError(uber::ErrorCode const code, uint64_t const requestId)
{
  GetPlatform().RunOnGuiThread([=]() {
    if (g_lastRequestId != requestId)
      return;

    JNIEnv * env = jni::GetEnv();

    static jclass const errCodeClass = env->FindClass("com/mapswithme/maps/uber/Uber$ErrorCode");
    ASSERT(errCodeClass, ());

    jobject const routingControllerInstance =
        env->CallStaticObjectMethod(g_routingControllerClass, g_routingControllerGetMethod);

    env->CallVoidMethod(routingControllerInstance, g_uberErrorCallbackMethod,
                        jni::ToJavaString(env, uber::DebugPrint(code)));
  });
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_com_mapswithme_maps_uber_Uber_nativeRequestUberProducts(
    JNIEnv * env, jclass clazz, jobject policy, jdouble srcLat, jdouble srcLon, jdouble dstLat,
    jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  g_lastRequestId =
      g_framework->RequestUberProducts(env, policy, from, to, &OnUberInfoReceived, &OnUberError);
}

JNIEXPORT jobject JNICALL Java_com_mapswithme_maps_uber_Uber_nativeGetUberLinks(
    JNIEnv * env, jclass clazz, jstring productId, jdouble srcLat, jdouble srcLon, jdouble dstLat,
    jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  uber::RideRequestLinks const links =
      android::Framework::GetUberLinks(jni::ToNativeString(env, productId), from, to);
  return env->NewObject(g_uberLinksClass, g_uberLinksConstructor,
                        jni::ToJavaString(env, links.m_deepLink),
                        jni::ToJavaString(env, links.m_universalLink));
}
}  // extern "C"
