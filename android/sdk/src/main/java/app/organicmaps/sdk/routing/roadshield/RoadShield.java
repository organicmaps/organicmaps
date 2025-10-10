package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public record RoadShield(@NonNull RoadShieldType type, @NonNull String text, @Nullable String additionalText)
{
  @Override
  @NonNull
  public String toString()
  {
    return "RoadShield{type=" + type + ", text='" + text + "', additionalText='" + additionalText + "'}";
  }
}
