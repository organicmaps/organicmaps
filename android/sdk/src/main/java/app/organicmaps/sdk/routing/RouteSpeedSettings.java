package app.organicmaps.sdk.routing;

import androidx.annotation.Nullable;

/**
 * The personal speed the user keeps and, for bicycles, the wind they ride in, together with the
 * limits the UI has to respect.
 */
public final class RouteSpeedSettings
{
  public static final int WIND_DIRECTION_STEP_DEGREES = 45;
  public static final int DEFAULT_WIND_SPEED_MPS = 3;

  public final double cruisingSpeedKmph;
  public final int windSpeedMps;
  public final int windDirectionDegrees;
  public final double minSpeedKmph;
  public final double maxSpeedKmph;
  public final double speedStepKmph;
  public final double defaultSpeedKmph;
  public final int maxWindSpeedMps;

  // Called from JNI.
  public RouteSpeedSettings(double cruisingSpeedKmph, int windSpeedMps, int windDirectionDegrees, double minSpeedKmph,
                            double maxSpeedKmph, double speedStepKmph, double defaultSpeedKmph, int maxWindSpeedMps)
  {
    this.cruisingSpeedKmph = cruisingSpeedKmph;
    this.windSpeedMps = windSpeedMps;
    this.windDirectionDegrees = windDirectionDegrees;
    this.minSpeedKmph = minSpeedKmph;
    this.maxSpeedKmph = maxSpeedKmph;
    this.speedStepKmph = speedStepKmph;
    this.defaultSpeedKmph = defaultSpeedKmph;
    this.maxWindSpeedMps = maxWindSpeedMps;
  }

  public boolean isWindSupported()
  {
    return maxWindSpeedMps > 0;
  }

  public int changedCount()
  {
    return (cruisingSpeedKmph == defaultSpeedKmph ? 0 : 1) + (windSpeedMps > 0 ? 1 : 0);
  }

  @Nullable
  public static native RouteSpeedSettings nativeGet();

  public static native void nativeSet(double cruisingSpeedKmph, int windSpeedMps, int windDirectionDegrees);
}
