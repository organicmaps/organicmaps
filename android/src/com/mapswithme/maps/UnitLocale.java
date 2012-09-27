package com.mapswithme.maps;

import java.util.Locale;

public class UnitLocale
{
  public static int METRIC = 0;
  public static int IMPERIAL = 1;

  public static int getCurrent()
  {
    final String code = Locale.getDefault().getCountry();
    // USA, UK, Liberia, Burma
    String arr[] = { "US", "GB", "LR", "MM" };
    for (String s : arr)
      if (s.equalsIgnoreCase(code))
        return IMPERIAL;

    return METRIC;
  }
}
