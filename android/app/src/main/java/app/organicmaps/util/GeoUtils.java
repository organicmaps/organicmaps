package app.organicmaps.util;

import androidx.annotation.NonNull;

import app.organicmaps.bookmarks.data.ParcelablePointD;

public class GeoUtils
{
  @NonNull
  static public ParcelablePointD toLatLon(double merX, double merY)
  {
    return nativeToLatLon(merX, merY);
  }

  @NonNull
  private static native ParcelablePointD nativeToLatLon(double merX, double merY);
}
