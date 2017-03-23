package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

interface AdTracker
{
  void start(@NonNull String bannerId);
  void stop(@NonNull String bannerId);
}
