package com.mapswithme.maps;

import android.support.annotation.NonNull;

public class FeatureId
{
  // Mwm base name.
  @NonNull
  public final String mCountryName;

  // Mwm version.
  public final long mVersion;

  // Feature index.
  public final int mIndex;

  public FeatureId(@NonNull String countryName, long version, int index)
  {
    mCountryName = countryName;
    mVersion = version;
    mIndex = index;
  }
};
