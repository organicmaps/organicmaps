package com.mapswithme.util;

import java.util.Locale;

import android.annotation.SuppressLint;
import android.content.Context;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

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
