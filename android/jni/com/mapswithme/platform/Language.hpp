#pragma once

#include "../core/jni_helper.hpp"

#include "std/string.hpp"

/// more details about deprecated language codes http://developer.android.com/reference/java/util/Locale.html
inline string ReplaceDeprecatedLanguageCode(JNIEnv * env, string const language)
{
  /// correct language codes & deprecated ones to be replaced
  static constexpr char * INDONESIAN_LANG_DEPRECATED = "in";
  static constexpr char * INDONESIAN_LANG = "id";
  static constexpr char * HEBREW_LANG_DEPRECATED = "iw";
  static constexpr char * HEBREW_LANG = "he";

  if (!language.compare(INDONESIAN_LANG_DEPRECATED))
    return INDONESIAN_LANG;
  else if (!language.compare(HEBREW_LANG_DEPRECATED))
    return HEBREW_LANG;

  return language;
}
