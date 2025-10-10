package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoutingInfo;

/// Information about road shields that are displayed in street name fields of {@link RoutingInfo}.
public class RoadShieldInfo
{
  /// An array of road shields for the target (next) street
  @Nullable
  public final RoadShield[] targetRoadShields;
  /// Position of the target road shields in the street name string. Inclusive
  public final int targetRoadShieldsIndexStart;
  /// Position of the target road shields in the street name string. Exclusive
  public final int targetRoadShieldsIndexEnd;
  /// An array of road shields for the junction
  @Nullable
  public final RoadShield[] junctionShields;
  /// Position of the junction road shields in the street name string. Inclusive
  public final int junctionShieldsIndexStart;
  /// Position of the junction road shields in the street name string. Exclusive
  public final int junctionShieldsIndexEnd;

  private RoadShieldInfo(@Nullable RoadShield[] targetRoadShields, int targetRoadShieldsIndexStart,
                         int targetRoadShieldsIndexEnd, @Nullable RoadShield[] junctionShields,
                         int junctionShieldsIndexStart, int junctionShieldsIndexEnd)
  {
    this.targetRoadShields = targetRoadShields;
    this.targetRoadShieldsIndexStart = targetRoadShieldsIndexStart;
    this.targetRoadShieldsIndexEnd = targetRoadShieldsIndexEnd;
    this.junctionShields = junctionShields;
    this.junctionShieldsIndexStart = junctionShieldsIndexStart;
    this.junctionShieldsIndexEnd = junctionShieldsIndexEnd;
  }

  @Override
  @NonNull
  public String toString()
  {
    StringBuilder sb = new StringBuilder("RoadShieldInfo{targetRoadShields=");
    if (targetRoadShields == null)
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
    sb.append(", junctionShields=");
    if (junctionShields == null)
      sb.append("null");
    else
    {
      sb.append("[");
      for (int i = 0; i < junctionShields.length; i++)
      {
        sb.append(junctionShields[i]);
        if (i != junctionShields.length - 1)
          sb.append(", ");
      }
      sb.append("], position=[")
          .append(junctionShieldsIndexStart)
          .append(", ")
          .append(junctionShieldsIndexEnd)
          .append(")");
    }
    sb.append("}");
    return sb.toString();
  }
}
