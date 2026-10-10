package app.organicmaps.sdk.settings;

import java.util.Locale;

public class UnitLocale
{
  // This constants should be equal with platform/settings.hpp
  public static final int UNITS_UNDEFINED = -1;
  public static final int UNITS_METRIC = 0;
  public static final int UNITS_FOOT = 1;

  private static int getDefaultUnits()
  {
    final String code = Locale.getDefault().getCountry();
    // USA, UK, Liberia, Burma
    String[] arr = {"US", "GB", "LR", "MM"};
    for (String s : arr)
      if (s.equalsIgnoreCase(code))
        return UNITS_FOOT;

    return UNITS_METRIC;
  }

  private static native int getCurrentUnits();

  private static native void setCurrentUnits(int u);

  private static native int getCurrentAltitudeUnits();

  private static native void setCurrentAltitudeUnits(int units);

  public static int getUnits()
  {
    return getCurrentUnits();
  }

  public static void setUnits(int units)
  {
    setCurrentUnits(units);
  }

  public static int getAltitudeUnits()
  {
    return getCurrentAltitudeUnits();
  }

  public static void setAltitudeUnits(int units)
  {
    setCurrentAltitudeUnits(units);
  }

  public static void initializeCurrentUnits()
  {
    final int u = getCurrentUnits();
    setCurrentUnits(u == UNITS_UNDEFINED ? getDefaultUnits() : u);
  }
}
