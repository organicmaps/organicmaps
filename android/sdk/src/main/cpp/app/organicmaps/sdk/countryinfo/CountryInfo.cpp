#include "CountryInfo.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace countryinfo
{
jobject CreateCountryInfo(JNIEnv * env, jstring countryId, jobject driverPosition)
{
  jclass countryInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/countryinfo/CountryInfo");
  jmethodID ctor = jni::GetConstructorID(env, countryInfoClass,
                                         "(Ljava/lang/String;Lapp/organicmaps/sdk/countryinfo/DriverPosition;)V");

  return env->NewObject(countryInfoClass, ctor, countryId, driverPosition);
}
}  // namespace countryinfo
