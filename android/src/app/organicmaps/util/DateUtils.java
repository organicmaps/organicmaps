package app.organicmaps.util;

import android.content.Context;

import androidx.annotation.NonNull;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public final class DateUtils
{
  private DateUtils()
  {
  }

  @NonNull
  public static DateFormat getMediumDateFormatter()
  {
    return DateFormat.getDateInstance(DateFormat.MEDIUM, Locale.getDefault());
  }

  @NonNull
  public static DateFormat getShortDateFormatter()
  {
    return DateFormat.getDateInstance(DateFormat.SHORT, Locale.getDefault());
  }

  public static boolean is24HourFormat(@NonNull Context context)
  {
    return android.text.format.DateFormat.is24HourFormat(context);
  }

  // Converts 220131 to locale-dependent date (e.g. 31 January 2022),
  @NonNull
  public static String getLocalDate(long v)
  {
    final SimpleDateFormat format = new SimpleDateFormat("yyMMdd", Locale.getDefault());
    final String strVersion = String.valueOf(v);
    try
    {
      final Date date = format.parse(strVersion);
      if (date == null)
        return strVersion;
      return java.text.DateFormat.getDateInstance().format(date);
    } catch (java.text.ParseException e)
    {
      e.printStackTrace();
      return strVersion;
    }
  }
}
