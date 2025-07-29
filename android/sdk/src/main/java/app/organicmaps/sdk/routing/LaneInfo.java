package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;

public final class LaneInfo
{
  public final LaneWay[] mLaneWays;
  public final LaneWay mActiveLaneWay;

  public LaneInfo(@NonNull LaneWay[] laneWays, LaneWay activeLane)
  {
    mLaneWays = laneWays;
    mActiveLaneWay = activeLane;
  }

  @NonNull
  @Override
  public String toString()
  {
    StringBuilder sb = new StringBuilder();
    sb.append("LaneInfo{activeLaneWay=").append(mActiveLaneWay.toString()).append(", laneWays=[");
    for (LaneWay i : mLaneWays)
      sb.append(" ").append(i);
    sb.append("]}");
    return sb.toString();
  }
}
