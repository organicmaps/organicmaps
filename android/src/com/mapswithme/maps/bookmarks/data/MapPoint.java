package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;

public interface MapPoint
{
  @NonNull
  String getTitle();

  double getLat();

  double getLon();

  double getScale();

  @NonNull
  String getAddress();
}
