#pragma once

#include <jni.h>

namespace countryinfo
{
jobject CreateCountryInfo(JNIEnv * env, jstring countryId, jobject driverPosition);
}  // namespace countryinfo
