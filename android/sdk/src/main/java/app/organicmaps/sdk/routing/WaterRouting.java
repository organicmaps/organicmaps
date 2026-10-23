package app.organicmaps.sdk.routing;

import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;
import app.organicmaps.sdk.util.Distance;

public final class WaterRouting
{
  private WaterRouting() {}

  @WorkerThread
  public static native boolean isPointEligible(double lat, double lon);

  // Water, land gaps, total, formatted using the current application units.
  @Nullable
  public static native Distance[] getDistances();
}
