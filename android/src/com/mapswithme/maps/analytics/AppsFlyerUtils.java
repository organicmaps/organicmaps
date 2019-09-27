package com.mapswithme.maps.analytics;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Map;

class AppsFlyerUtils
{
  private AppsFlyerUtils()
  {
    // Utility class.
  }

  static boolean isFirstLaunch(@NonNull Map<String, String> conversionData)
  {
    String isFirstLaunch = conversionData.get("is_first_launch");
    return Boolean.parseBoolean(isFirstLaunch);
  }

  @Nullable
  static String getDeepLink(@NonNull Map<String, String> conversionData)
  {
    return conversionData.get("af_dp");
  }
}
