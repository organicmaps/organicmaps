#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/country_file.hpp"

extern "C"
{
// static int nativeGetDrivingSide(@NonNull String countryId);
JNIEXPORT jint Java_app_organicmaps_sdk_CountryMetadata_nativeGetDrivingSide(JNIEnv * env, jclass, jstring countryId)
{
  std::string const countryName = jni::ToNativeString(env, countryId);
  auto const mwmId = frm()->GetDataSource().GetMwmIdByCountryFile(platform::CountryFile(countryName));
  return mwmId.GetInfo()->GetRegionData().IsRightDrivingSide() ? 1 : 0;
}
}
