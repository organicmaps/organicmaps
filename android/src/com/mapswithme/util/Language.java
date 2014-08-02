package com.mapswithme.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import java.util.Locale;

public class Language
{
  // Locale.getLanguage() returns even 3-letter codes, not that we need in the C++ core,
  // so we use locale itself, like zh_CN
  static public String getDefault()
  {
    return Locale.getDefault().toString();
  }

  // After some testing on Galaxy S4, looks like this method doesn't work on all devices:
  // sometime it always returns the same value as getDefault()
  static public String getKeyboardInput(Context context)
  {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB)
    {
      final InputMethodManager imm = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
      if (imm != null)
      {
        final InputMethodSubtype ims = imm.getCurrentInputMethodSubtype();
        if (ims != null)
          return ims.getLocale();
      }
    }

    return getDefault();
  }
}
