package app.organicmaps.sdk;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.routing.RouteMarkType;

/** Immutable main-thread snapshot of a route mark and its map-surface pixel position. */
@Keep
public final class MapRoutePoint
{
  public final long mId;
  @NonNull
  public final String mTitle;
  public final RouteMarkType mType;
  public final int mIntermediateIndex;
  public final float mX;
  public final float mY;
  public final double mLat;
  public final double mLon;

  // Called from JNI. Types match RouteMarkType: start = 0, intermediate = 1, finish = 2.
  public MapRoutePoint(long id, @NonNull String title, int type, int intermediateIndex, float x, float y, double lat,
                       double lon)
  {
    mId = id;
    mTitle = title;
    mType = RouteMarkType.values()[type];
    mIntermediateIndex = intermediateIndex;
    mX = x;
    mY = y;
    mLat = lat;
    mLon = lon;
  }
}
