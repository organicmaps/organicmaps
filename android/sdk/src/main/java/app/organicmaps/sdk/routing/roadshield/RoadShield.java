package app.organicmaps.sdk.routing.roadshield;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class RoadShield
{
  @NonNull
  public final RoadShieldType type;
  @NonNull
  public final String text;
  @Nullable
  public final String additionalText;

  private RoadShield(@NonNull RoadShieldType type, @NonNull String text, @Nullable String additionalText)
  {
    this.type = type;
    this.text = text;
    this.additionalText = additionalText;
  }

  @Override
  @NonNull
  public String toString()
  {
    return "RoadShield{type=" + type + ", text='" + text + "', additionalText='" + additionalText + "'}";
  }
}
