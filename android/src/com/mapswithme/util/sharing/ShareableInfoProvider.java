package com.mapswithme.util.sharing;

import androidx.annotation.NonNull;

public interface ShareableInfoProvider
{
  @NonNull
  String getName();

  double getLat();

  double getLon();

  double getScale();

  @NonNull
  String getAddress();
}
