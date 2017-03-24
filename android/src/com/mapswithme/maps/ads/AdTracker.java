package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

public interface AdTracker
{
  void onViewShown(@NonNull String bannerId);
  void onViewHidden(@NonNull String bannerId);
  void onContentObtained(@NonNull String bannerId);
  boolean isImpressionGood(@NonNull String bannerId);
}
