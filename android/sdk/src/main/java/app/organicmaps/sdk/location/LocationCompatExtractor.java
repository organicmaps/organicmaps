package app.organicmaps.sdk.location;

import android.location.Location;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.location.LocationCompat;
import app.organicmaps.sdk.util.Assert;

public class LocationCompatExtractor
{
  public record Altitude(double altitude, float accuracy)
  {
    public Altitude
    {
      Assert.debug(Float.isFinite(accuracy) && accuracy >= 0, "Altitude accuracy must be finite and non-negative");
    }
  }
  public record Speed(float speed, float accuracy)
  {
    public Speed
    {
      Assert.debug(speed >= 0, "Speed must be non-negative");
    }
  }
  public record Bearing(float bearing, float accuracy)
  {
    public Bearing
    {
      Assert.debug(bearing >= 0, "Bearing must be non-negative");
    }
  }

  private static float getDefaultAltitudeAccuracy(@NonNull Location location)
  {
    if (!location.hasAccuracy())
      return 20.f;
    float accuracy = location.getAccuracy() * 2;
    return Float.isFinite(accuracy) && accuracy > 0 ? accuracy : 20.f;
  }

  /** Returns MSL altitude added by the provider or {@link LocationHelper}. */
  @Nullable
  public static Altitude getAltitude(@NonNull Location location)
  {
    if (!LocationCompat.hasMslAltitude(location))
      return null;
    double altitude = LocationCompat.getMslAltitudeMeters(location);
    if (!Double.isFinite(altitude))
      return null;
    float accuracy =
        LocationCompat.hasMslAltitudeAccuracy(location) ? LocationCompat.getMslAltitudeAccuracyMeters(location) : -1;
    if (!Float.isFinite(accuracy) || accuracy < 0)
      accuracy = getDefaultAltitudeAccuracy(location);
    return new Altitude(altitude, accuracy);
  }

  @Nullable
  static Speed getSpeed(@NonNull Location location)
  {
    if (!location.hasSpeed())
      return null;

    if (Build.VERSION.SDK_INT >= 26 && location.hasSpeedAccuracy())
      return new Speed(location.getSpeed(), location.getSpeedAccuracyMetersPerSecond());

    // Default accuracy value when real accuracy is not available.
    return new Speed(location.getSpeed(), 5.0f);
  }

  @Nullable
  static Bearing getBearing(@NonNull Location location)
  {
    if (!location.hasBearing())
      return null;

    if (Build.VERSION.SDK_INT >= 26 && location.hasBearingAccuracy())
      return new Bearing(location.getBearing(), location.getBearingAccuracyDegrees());

    // Default accuracy value when real accuracy is not available.
    return new Bearing(location.getBearing(), 10.0f);
  }
}
