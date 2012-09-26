#pragma once

#include "../std/string.hpp"

namespace languages
{

/// @note This functions are heavy enough to call them often. Be careful.
//@{
/// @return system language preferences in the form "en|ru|es|zh"
string PreferredLanguages();
/// @return language code for current user in the form "en"
string CurrentLanguage();
//@}

}
