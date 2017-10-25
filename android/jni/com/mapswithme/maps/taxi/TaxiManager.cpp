#include "android/jni/com/mapswithme/maps/Framework.hpp"
#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include "partners_api/taxi_provider.hpp"

namespace
{
jclass g_productClass;
jclass g_taxiManagerClass;
jclass g_taxiInfoClass;
jclass g_taxiInfoErrorClass;
jmethodID g_taxiInfoConstructor;
jmethodID g_taxiInfoErrorConstructor;
jobject g_taxiManagerInstance;
jmethodID g_productConstructor;
jfieldID g_taxiManagerInstanceField;
jmethodID g_taxiInfoCallbackMethod;
jmethodID g_taxiErrorCallbackMethod;
jclass g_taxiLinksClass;
jmethodID g_taxiLinksConstructor;
uint64_t g_lastRequestId;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_taxiManagerClass)
    return;

  g_taxiManagerClass =
      jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiManager");
  g_productClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfo$Product");
  g_productConstructor = jni::GetConstructorID(
      env, g_productClass,
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  g_taxiInfoClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfo");
  g_taxiInfoErrorClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiInfoError");
  g_taxiManagerInstanceField = jni::GetStaticFieldID(
      env, g_taxiManagerClass, "INSTANCE", "Lcom/mapswithme/maps/taxi/TaxiManager;");
  g_taxiManagerInstance =
      env->GetStaticObjectField(g_taxiManagerClass, g_taxiManagerInstanceField);
  g_taxiInfoCallbackMethod =
      jni::GetMethodID(env, g_taxiManagerInstance, "onTaxiProvidersReceived",
                       "([Lcom/mapswithme/maps/taxi/TaxiInfo;)V");
  g_taxiErrorCallbackMethod = jni::GetMethodID(env, g_taxiManagerInstance,
                                               "onTaxiErrorsReceived", "([Lcom/mapswithme/maps/taxi/TaxiInfoError;)V");
  g_taxiInfoConstructor = jni::GetConstructorID(env, g_taxiInfoClass,
                                                "(I[Lcom/mapswithme/maps/taxi/TaxiInfo$Product;)V");
  g_taxiInfoErrorConstructor = jni::GetConstructorID(env, g_taxiInfoErrorClass,
                                                  "(ILjava/lang/String;)V");
  g_taxiLinksClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/taxi/TaxiLinks");
  g_taxiLinksConstructor =
      jni::GetConstructorID(env, g_taxiLinksClass, "(Ljava/lang/String;Ljava/lang/String;)V");
}

void OnTaxiInfoReceived(taxi::ProvidersContainer const & providers, uint64_t const requestId)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  auto const productBuilder = [](JNIEnv * env, taxi::Product const & item)
  {
    jni::TScopedLocalRef jProductId(env, jni::ToJavaString(env, item.m_productId));
    jni::TScopedLocalRef jName(env, jni::ToJavaString(env, item.m_name));
    jni::TScopedLocalRef jTime(env, jni::ToJavaString(env, item.m_time));
    jni::TScopedLocalRef jPrice(env, jni::ToJavaString(env, item.m_price));
    jni::TScopedLocalRef jCurrency(env, jni::ToJavaString(env, item.m_currency));
    return env->NewObject(g_productClass, g_productConstructor, jProductId.get(), jName.get(),
                          jTime.get(), jPrice.get(), jCurrency.get());
  };

  auto const providerBuilder = [productBuilder](JNIEnv * env, taxi::Provider const & item)
  {
    return env->NewObject(g_taxiInfoClass, g_taxiInfoConstructor, item.GetType(),
                          jni::ToJavaArray(env, g_productClass, item.GetProducts(), productBuilder));
  };

  jni::TScopedLocalObjectArrayRef jProviders(env, jni::ToJavaArray(env, g_taxiInfoClass, providers,
                                             providerBuilder));

  jobject const taxiManagerInstance = env->GetStaticObjectField(g_taxiManagerClass,
                                                                g_taxiManagerInstanceField);
  env->CallVoidMethod(taxiManagerInstance, g_taxiInfoCallbackMethod, jProviders.get());

  jni::HandleJavaException(env);
}

void OnTaxiError(taxi::ErrorsContainer const & errors, uint64_t const requestId)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  jobject const taxiManagerInstance = env->GetStaticObjectField(g_taxiManagerClass,
                                                                 g_taxiManagerInstanceField);

  auto const errorBuilder = [](JNIEnv * env, taxi::ProviderError const & error)
  {
    jni::TScopedLocalRef jErrorCode(env, jni::ToJavaString(env, taxi::DebugPrint(error.m_code)));
    return env->NewObject(g_taxiInfoErrorClass, g_taxiInfoErrorConstructor, error.m_type,
                          jErrorCode.get());
  };


  jni::TScopedLocalObjectArrayRef jErrors(env, jni::ToJavaArray(env, g_taxiInfoErrorClass, errors, errorBuilder));

  env->CallVoidMethod(taxiManagerInstance, g_taxiErrorCallbackMethod, jErrors.get());

  jni::HandleJavaException(env);
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL Java_com_mapswithme_maps_taxi_TaxiManager_nativeRequestTaxiProducts(
    JNIEnv * env, jclass clazz, jobject policy, jdouble srcLat, jdouble srcLon, jdouble dstLat,
    jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  g_lastRequestId =
      g_framework->RequestTaxiProducts(env, policy, from, to, &OnTaxiInfoReceived, &OnTaxiError);
}

JNIEXPORT jobject JNICALL Java_com_mapswithme_maps_taxi_TaxiManager_nativeGetTaxiLinks(
    JNIEnv * env, jclass clazz, jobject policy, jint providerType, jstring productId, jdouble srcLat,
    jdouble srcLon, jdouble dstLat, jdouble dstLon)
{
  PrepareClassRefs(env);

  ms::LatLon const from(srcLat, srcLon);
  ms::LatLon const to(dstLat, dstLon);

  taxi::Provider::Type type = static_cast<taxi::Provider::Type>(providerType);
  taxi::RideRequestLinks const links =
    g_framework->GetTaxiLinks(env, policy, type, jni::ToNativeString(env, productId), from, to);

  return env->NewObject(g_taxiLinksClass, g_taxiLinksConstructor,
                        jni::ToJavaString(env, links.m_deepLink),
                        jni::ToJavaString(env, links.m_universalLink));
}
}  // extern "C"
