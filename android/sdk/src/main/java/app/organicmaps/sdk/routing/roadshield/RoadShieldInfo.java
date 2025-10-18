package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutingInfo;

/// Information about road shields that are displayed in street name fields of {@link RoutingInfo}.
public final class RoadShieldInfo
{
  /// An array of road shields for the target (next) street
  @Nullable
  public final RoadShield[] targetRoadShields;
  /// Start position of the target road shields in the street name string. Inclusive
  public final int targetRoadShieldsIndexStart;
  /// End position of the target road shields in the street name string. Exclusive
  public final int targetRoadShieldsIndexEnd;
  /// Start position of the junction info in the street name string. Inclusive
  public final int junctionInfoIndexStart;
  /// End position of the junction info in the street name string. Exclusive
  public final int junctionInfoIndexEnd;

  // Used by JNI.
  @Keep
  private RoadShieldInfo(@Nullable RoadShield[] targetRoadShields, int targetRoadShieldsIndexStart,
                         int targetRoadShieldsIndexEnd, int junctionInfoIndexStart, int junctionInfoIndexEnd)
  {
    this.targetRoadShields = targetRoadShields;
    this.targetRoadShieldsIndexStart = targetRoadShieldsIndexStart;
    this.targetRoadShieldsIndexEnd = targetRoadShieldsIndexEnd;
    this.junctionInfoIndexStart = junctionInfoIndexStart;
    this.junctionInfoIndexEnd = junctionInfoIndexEnd;
  }

  public boolean hasTargetRoadShields()
  {
    return targetRoadShields != null && targetRoadShieldsIndexStart != targetRoadShieldsIndexEnd;
  }

  public boolean hasJunctionInfo()
  {
    return junctionInfoIndexStart != junctionInfoIndexEnd;
  }

  @Override
  @NonNull
  public String toString()
  {
    StringBuilder sb = new StringBuilder("RoadShieldInfo{targetRoadShields=");
    if (!hasTargetRoadShields())
      sb.append("null");
    else
    {
      sb.append("[");
      for (int i = 0; i < targetRoadShields.length; i++)
      {
        sb.append(targetRoadShields[i]);
        if (i != targetRoadShields.length - 1)
          sb.append(", ");
      }
      sb.append("], position=[")
          .append(targetRoadShieldsIndexStart)
          .append(", ")
          .append(targetRoadShieldsIndexEnd)
          .append(")");
    }
    sb.append(", junctionInfo=");
    if (!hasJunctionInfo())
      sb.append("null");
    else
      sb.append("[").append(junctionInfoIndexStart).append(", ").append(junctionInfoIndexEnd).append(")");
    sb.append("}");
    return sb.toString();
  }
}
