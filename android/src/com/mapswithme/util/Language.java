package com.mapswithme.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import java.util.Locale;

public class Language
{
  static public String getDefault()
  {
    return Locale.getDefault().getLanguage();
  }

  @SuppressLint("NewApi")
  static public String getKeyboardInput(Context context)
  {
    if (Utils.apiEqualOrGreaterThan(11))
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
