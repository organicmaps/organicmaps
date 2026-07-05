package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;

public final class LaneInfo
{
  public final LaneWay[] mLaneWays;
  public final LaneWay mActiveLaneWay;
  // How many adjacent identical physical lanes this entry stands for (see the C++
  // routing::turns::lanes::CollapseLanes); render one icon with a count badge when > 1.
  public final int mSimilarLanesCount;

  public LaneInfo(@NonNull LaneWay[] laneWays, LaneWay activeLane)
  {
    this(laneWays, activeLane, 1);
  }

  public LaneInfo(@NonNull LaneWay[] laneWays, LaneWay activeLane, int similarLanesCount)
  {
    mLaneWays = laneWays;
    mActiveLaneWay = activeLane;
    mSimilarLanesCount = similarLanesCount;
  }

  @NonNull
  @Override
  public String toString()
  {
    StringBuilder sb = new StringBuilder();
    sb.append("LaneInfo{activeLaneWay=").append(mActiveLaneWay.toString()).append(", laneWays=[");
    for (LaneWay i : mLaneWays)
      sb.append(" ").append(i);
    sb.append("], similarLanesCount=").append(mSimilarLanesCount).append("}");
    return sb.toString();
  }
}
