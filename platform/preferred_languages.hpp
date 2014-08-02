#pragma once

#include "../std/string.hpp"

namespace languages
{

/// @note This functions are heavy enough to call them often. Be careful.
//@{
/// @return system language preferences in the form "en|ru|es|zh"
string PreferredLanguages();
/// @return language code for current user in the form "en"
/// Returned languages are normalized to our supported languages in the core, see multilang_utf8_string.cpp
/// and should not be used with any sub-locales like zh-Hans/zh-Hant.
/// Some langs like Danish (da) are not supported too in the core
string CurrentLanguage();
//@}

}
