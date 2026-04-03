package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

public class RouteAltitudeData
{
  @NonNull
  private final double[] mDistances; // meters from start
  @NonNull
  private final int[] mAltitudes; // meters elevation

  private final int mTotalAscent;
  private final int mTotalDescent;
  private final int mMinAltitude;
  private final int mMaxAltitude;

  // Called from JNI.
  @Keep
  public RouteAltitudeData(@NonNull double[] distances, @NonNull int[] altitudes, int totalAscent, int totalDescent,
                           int minAltitude, int maxAltitude)
  {
    if (distances.length != altitudes.length)
      throw new IllegalArgumentException("Arrays must have the same length");
    mDistances = distances;
    mAltitudes = altitudes;
    mTotalAscent = totalAscent;
    mTotalDescent = totalDescent;
    mMinAltitude = minAltitude;
    mMaxAltitude = maxAltitude;
  }

  public int getSize()
  {
    return mDistances.length;
  }

  public double getDistance(int index)
  {
    return mDistances[index];
  }

  public int getAltitude(int index)
  {
    return mAltitudes[index];
  }

  public int getTotalAscent()
  {
    return mTotalAscent;
  }

  public int getTotalDescent()
  {
    return mTotalDescent;
  }

  public int getMinAltitude()
  {
    return mMinAltitude;
  }

  public int getMaxAltitude()
  {
    return mMaxAltitude;
  }
}
