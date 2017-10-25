#pragma once

#include "android/jni/com/mapswithme/core/jni_helper.hpp"

#include <string>

/// More details about deprecated language codes http://developer.android.com/reference/java/util/Locale.html
std::string ReplaceDeprecatedLanguageCode(std::string const & language);
