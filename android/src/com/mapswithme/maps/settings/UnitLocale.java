package com.mapswithme.maps.settings;

import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.preference.ListPreference;
import android.util.AttributeSet;

import com.mapswithme.maps.R;

public class UnitLocale extends ListPreference
{
  public UnitLocale(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  // / @note This constants should be equal with platform/settings.hpp
  public static final int UNITS_UNDEFINED = -1;
  public static final int UNITS_METRIC = 0;
  public static final int UNITS_YARD = 1;
  public static final int UNITS_FOOT = 2;

  private static int getDefaultUnits()
  {
    final String code = Locale.getDefault().getCountry();
    // USA, UK, Liberia, Burma
    String arr[] =
    { "US", "GB", "LR", "MM" };
    for (String s : arr)
      if (s.equalsIgnoreCase(code))
        return UNITS_FOOT;

    return UNITS_METRIC;
  }

  private static native int getCurrentUnits();

  private static native void setCurrentUnits(int u);

  public static int getUnits()
  {
    return getCurrentUnits();
  }

  public static void setUnits(int units)
  {
    setCurrentUnits(units);
  }

  public static void initializeCurrentUnits()
  {
    final int u = getCurrentUnits();
    if (u == UNITS_UNDEFINED)
    {
      // Checking system-default measurement system
      setCurrentUnits(getDefaultUnits());
    } 
    else
    {
      setCurrentUnits(u);
    }
  }
}
