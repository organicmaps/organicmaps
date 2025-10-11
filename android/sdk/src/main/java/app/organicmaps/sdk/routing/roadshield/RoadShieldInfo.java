package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public record RoadShieldInfo(@Nullable RoadShield[] targetRoadShields, @Nullable RoadShield[] junctionShields)
{
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
      sb.append("]");
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
      sb.append("]");
    }
    sb.append("}");
    return sb.toString();
  }
}
