#pragma once

#include <jni.h>

#include <string_view>

namespace countryinfo
{
jobject CreateDriverPosition(JNIEnv * env, std::string_view const & position);
}  // namespace countryinfo
