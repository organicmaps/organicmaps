package com.mapswithme.util;

import android.support.annotation.NonNull;

import java.text.DateFormat;
import java.util.Locale;

public class DateUtils
{
  private DateUtils()
  {
  }

  @NonNull
  public static DateFormat getMediumDateFormat()
  {
    return DateFormat.getDateInstance(DateFormat.MEDIUM, Locale.getDefault());
  }
}
