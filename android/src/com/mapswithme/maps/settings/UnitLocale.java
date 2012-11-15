package com.mapswithme.maps.settings;

import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;

import com.mapswithme.maps.R;

public class UnitLocale
{
  /// @note This constants should be equal with platform/settings.hpp
  public static final int UNITS_UNDEFINED = -1;
  public static final int UNITS_METRIC = 0;
  public static final int UNITS_YARD = 1;
  public static final int UNITS_FOOT = 2;

  public static int getDefaultUnits()
  {
    final String code = Locale.getDefault().getCountry();
    // USA, UK, Liberia, Burma
    String arr[] = { "US", "GB", "LR", "MM" };
    for (String s : arr)
      if (s.equalsIgnoreCase(code))
        return UNITS_FOOT;

    return UNITS_METRIC;
  }

  private static native int getCurrentUnits();
  private static native void setCurrentUnits(int u);

  public static void showUnitsSelectDlg(Activity parent)
  {
    new AlertDialog.Builder(parent)
    .setCancelable(false)
    .setMessage(parent.getString(R.string.which_measurement_system))
    .setNegativeButton(parent.getString(R.string.miles), new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dialog, int which)
      {
        dialog.dismiss();
        setCurrentUnits(UNITS_FOOT);
      }
    })
    .setPositiveButton(parent.getString(R.string.kilometres), new DialogInterface.OnClickListener()
    {
      @Override
      public void onClick(DialogInterface dlg, int which)
      {
        dlg.dismiss();
        setCurrentUnits(UNITS_METRIC);
      }
    })
    .create()
    .show();
  }

  public static void initializeCurrentUnits(Activity parent)
  {
    final int u = getCurrentUnits();
    if (u == UNITS_UNDEFINED)
    {
      // Checking system-default measurement system
      if (getDefaultUnits() == UNITS_METRIC)
      {
        setCurrentUnits(UNITS_METRIC);
      }
      else
      {
        showUnitsSelectDlg(parent);
      }
    }
    else
    {
      setCurrentUnits(u);
    }
  }
}
