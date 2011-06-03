#pragma once

#include "../std/string.hpp"

namespace languages
{

/// @return system language preferences in the form "en|ru|es|zh"
string PreferredLanguages();
/// @return language code for current user in the form "en"
string CurrentLanguage();

}
