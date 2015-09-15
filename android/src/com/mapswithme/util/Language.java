package com.mapswithme.util;

import android.content.Context;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import com.mapswithme.maps.MwmApplication;

import java.util.Locale;

public class Language
{
  private final static String TAG = "Language";
  // Locale.getLanguage() returns even 3-letter codes, not that we need in the C++ core,
  // so we use locale itself, like zh_CN
  public static String getDefaultLocale()
  {
    return Locale.getDefault().toString();
  }

  // After some testing on Galaxy S4, looks like this method doesn't work on all devices:
  // sometime it always returns the same value as getDefaultLocale()
  public static String getKeyboardLocale()
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
    {
      final InputMethodManager imm = (InputMethodManager) MwmApplication.get().getSystemService(Context.INPUT_METHOD_SERVICE);
      if (imm != null)
      {
        final InputMethodSubtype ims = imm.getCurrentInputMethodSubtype();
        if (ims != null)
          return ims.getLocale();
      }
    }

    return getDefaultLocale();
  }

  // Converts Locale to twine language name.
  // If locale can be converted returns a twine language name. For example zh-Hans, ru, en and so on.
  // If not returns an empty string.
  public static String localeToTwineLanguage(Locale locale)
  {
    if (locale == null)
    {
      Log.e(TAG, "localeToTwineLanguage was called with null Locale.");
      return "";
    }

    final String chinese = Locale.CHINESE.getLanguage();
    final String language = locale.getLanguage();
    final String country = locale.getCountry();

    if (chinese == null || language == null || country == null)
    {
      Log.e(TAG, "Methods Locale.getLanguage or Locale.getCountry return null.");
      return "";
    }

    if (chinese.equals(language))
    {
      if (country.equals("TW") || country.equals("MO") || country.equals("HK"))
      {
        return "zh-Hant"; // Chinese traditional
      }
      return "zh-Hans"; // Chinese simplified
    }
    if (TextUtils.isEmpty(language))
    {
      Log.e(TAG, "locale.getLanguage() returns null or empty string.");
      return "";
    }
    return language;
  }
}
