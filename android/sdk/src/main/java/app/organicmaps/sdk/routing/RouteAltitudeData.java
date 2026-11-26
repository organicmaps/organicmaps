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

  /** Directed sample indexes of the followed section. A fractional entry interpolates adjacent samples. */
  public final double entryPointIndex;
  /** A negative index denotes an ordinary route chart without a selected section. */
  public final int finishPointIndex;

  // Called from JNI.
  @Keep
  public RouteAltitudeData(@NonNull double[] distances, @NonNull int[] altitudes, int totalAscent, int totalDescent,
                           double entryPointIndex, int finishPointIndex)
  {
    if (distances.length != altitudes.length)
      throw new IllegalArgumentException("Arrays must have the same length");
    this.entryPointIndex = entryPointIndex;
    this.finishPointIndex = finishPointIndex;
    mDistances = distances;
    mAltitudes = altitudes;
    mTotalAscent = totalAscent;
    mTotalDescent = totalDescent;
  }

  public int getSize()
  {
    return mDistances.length;
  }

  /** @return the distances in ascending order, without a copy; do not modify them. */
  @NonNull
  public double[] getDistances()
  {
    return mDistances;
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
}
