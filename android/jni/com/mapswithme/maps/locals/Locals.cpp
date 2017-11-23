#include "android/jni/com/mapswithme/core/jni_helper.hpp"
#include "android/jni/com/mapswithme/maps/Framework.hpp"

#include <string>
#include <vector>

namespace
{
jclass g_localsClass = nullptr;
jobject g_localsInstance;
jmethodID g_onLocalsReceivedMethod;
jmethodID g_onLocalsErrorReceivedMethod;
jclass g_localExpertClass;
jmethodID g_localExpertConstructor;
jclass g_localErrorClass;
jmethodID g_localErrorConstructor;
uint64_t g_lastRequestId = 0;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_localsClass != nullptr)
    return;

  g_localsClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/Locals");
  static jfieldID const localsInstanceField = jni::GetStaticFieldID(env, g_localsClass, "INSTANCE",
                                                                    "Lcom/mapswithme/maps/discovery/Locals;");
  g_localsInstance = env->GetStaticObjectField(g_localsClass, localsInstanceField);
  g_onLocalsReceivedMethod = jni::GetMethodID(env, g_localsInstance, "onLocalsReceived",
                                              "([Lcom/mapswithme/maps/discovery/LocalExpert;)V");
  g_onLocalsErrorReceivedMethod = jni::GetMethodID(env, g_localsInstance,
                                                   "onLocalsErrorReceived",
                                                   "(Lcom/mapswithme/maps/discovery/LocalsError;)V");

  // int id, @NonNull String name, @NonNull String country,
  // @NonNull String city, double rating, int reviewCount,
  // double price, @NonNull String currency, @NonNull String motto,
  // @NonNull String about, @NonNull String offer, @NonNull String pageUrl,
  // @NonNull String photoUrl
  g_localExpertClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/LocalExpert");
  g_localExpertConstructor =
      jni::GetConstructorID(env, g_localExpertClass,
                            "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;DID"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  // @ErrorCode int code, @NonNull String message
  g_localErrorClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/discovery/LocalError");
  g_localErrorConstructor = jni::GetConstructorID(env, g_localErrorClass,
                                                  "(ILjava/lang/String;)V");
}

void OnLocalsSuccess(uint64_t requestId, std::vector<locals::LocalExpert> const & locals,
                     size_t pageNumber, size_t countPerPage, bool hasPreviousPage,
                     bool hasNextPage)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  auto const localExpertBuilder = [](JNIEnv * env, locals::LocalExpert const & expert)
  {
    jni::TScopedLocalRef jName(env, jni::ToJavaString(env, expert.m_name));
    jni::TScopedLocalRef jCountry(env, jni::ToJavaString(env, expert.m_country));
    jni::TScopedLocalRef jCity(env, jni::ToJavaString(env, expert.m_city));
    jni::TScopedLocalRef jCurrency(env, jni::ToJavaString(env, expert.m_currency));
    jni::TScopedLocalRef jMotto(env, jni::ToJavaString(env, expert.m_motto));
    jni::TScopedLocalRef jAboutExpert(env, jni::ToJavaString(env, expert.m_aboutExpert));
    jni::TScopedLocalRef jOfferDescription(env, jni::ToJavaString(env, expert.m_offerDescription));
    jni::TScopedLocalRef jPageUrl(env, jni::ToJavaString(env, expert.m_pageUrl));
    jni::TScopedLocalRef jPhotoUrl(env, jni::ToJavaString(env, expert.m_photoUrl));

    return env->NewObject(g_localExpertClass, g_localExpertConstructor,
                          expert.m_id, jName.get(), jCountry.get(), jCity.get(),
                          expert.m_rating, expert.m_reviewCount, expert.m_pricePerHour,
                          jCurrency.get(), jMotto.get(), jAboutExpert.get(),
                          jOfferDescription.get(), jPageUrl.get(), jPhotoUrl.get());
  };

  jni::TScopedLocalObjectArrayRef jLocals(env, jni::ToJavaArray(env, g_localExpertClass, locals,
                                                                localExpertBuilder));

  env->CallVoidMethod(g_localsInstance, g_onLocalsReceivedMethod, jLocals.get());

  jni::HandleJavaException(env);
}

void OnLocalsError(uint64_t requestId, int errorCode, std::string const & errorMessage)
{
  if (g_lastRequestId != requestId)
    return;

  JNIEnv * env = jni::GetEnv();

  jni::TScopedLocalRef errorStr(env, jni::ToJavaString(env, errorMessage));
  jni::TScopedLocalRef errorObj(env, env->NewObject(g_localErrorClass, g_localErrorConstructor,
                                                    errorCode, errorStr.get()));

  env->CallVoidMethod(g_localsInstance, g_onLocalsErrorReceivedMethod, errorObj.get());

  jni::HandleJavaException(env);
}
}  // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_locals_Locals_nativeRequestLocals(JNIEnv * env, jclass clazz,
                                                           jobject policy, jdouble lat,
                                                           jdouble lon)
{
  PrepareClassRefs(env);
  g_lastRequestId = g_framework->GetLocals(env, policy, lat, lon, &OnLocalsSuccess,
                                           &OnLocalsError);
}
}  // extern "C"
