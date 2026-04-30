package app.organicmaps.sdk.api;

import androidx.annotation.Keep;

/**
 * Represents url_scheme::RoutePoint from core.
 */
// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class RoutePoint
{
  public final double mLat;
  public final double mLon;
  public final String mName;
  public final String mCallback;
  public final boolean mIsMyPosition;

  public RoutePoint(double lat, double lon, String name, String callback, boolean isMyPosition)
  {
    mLat = lat;
    mLon = lon;
    mName = name;
    mCallback = callback;
    mIsMyPosition = isMyPosition;
  }
}
