package app.organicmaps.sdk.routing;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.Distance;

public final class WaterRouting
{
  private WaterRouting() {}

  public static native boolean isShortestConfirmed();

  // Water, land gaps, total, formatted using the current application units.
  @Nullable
  public static native Distance[] getDistances();
}
