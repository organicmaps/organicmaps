#include <jni.h>

#include "CountryInfo.hpp"
#include "DriverPosition.hpp"

namespace
{
jobject g_countryChangedListener = nullptr;
}

extern "C"
{
JNIEXPORT void JNICALL Java_app_organicmaps_sdk_countryinfo_CurrentCountryInfoManagerImpl_nativeSubscribe(
    JNIEnv * env, jclass, jobject listener)
{
  ASSERT(!g_countryChangedListener, ());
  g_countryChangedListener = env->NewGlobalRef(listener);

  auto const callback = [](storage::CountryId const & countryId)
  {
    if (countryId.empty())
      return;
    auto const mwmId =
        g_framework->NativeFramework()->GetDataSource().GetMwmIdByCountryFile(platform::CountryFile(countryId));
    std::string_view const driverPosition = mwmId.GetInfo()->GetRegionData().Get(feature::RegionData::RD_DRIVING);

    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetMethodID(env, g_countryChangedListener, "onCurrentCountryChanged",
                                          "(Lapp/organicmaps/sdk/countryinfo/CountryInfo;)V");
    env->CallVoidMethod(g_countryChangedListener, methodID,
                        jni::TScopedLocalRef(
                            env, countryinfo::CreateCountryInfo(env, jni::ToJavaString(env, countryId),
                                                                countryinfo::CreateDriverPosition(env, driverPosition)))
                            .get());
  };

  g_framework->NativeFramework()->SetCurrentCountryChangedListener(callback);
}

JNIEXPORT void JNICALL
Java_app_organicmaps_sdk_countryinfo_CurrentCountryInfoManagerImpl_nativeUnsubscribe(JNIEnv * env, jclass)
{
  g_framework->NativeFramework()->SetCurrentCountryChangedListener(nullptr);

  env->DeleteGlobalRef(g_countryChangedListener);
  g_countryChangedListener = nullptr;
}
}
