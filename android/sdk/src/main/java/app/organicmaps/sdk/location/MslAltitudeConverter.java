package app.organicmaps.sdk.location;

import android.content.Context;
import android.location.Location;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.core.location.LocationCompat;
import app.organicmaps.sdk.util.Assert;
import java.io.IOException;
import java.io.InputStream;

/** Adds MSL metadata synchronously on the main thread, preserving the provider's ellipsoidal altitude. */
final class MslAltitudeConverter
{
  private static final String ASSET_PATH = "geoid_model.bin";
  private static final double RMSE_METERS = 0.27;

  @Nullable
  private static Geoid sGeoid;

  private MslAltitudeConverter() {}

  @UiThread
  static void addMslAltitude(@NonNull Context context, @NonNull Location location)
  {
    if (LocationCompat.hasMslAltitude(location) && Double.isFinite(LocationCompat.getMslAltitudeMeters(location)))
      return;

    if (!location.hasAltitude() || !Double.isFinite(location.getAltitude()))
      return;

    double latitude = location.getLatitude();
    double longitude = location.getLongitude();
    Assert.debug(latitude >= -90 && latitude <= 90 && longitude >= -180 && longitude <= 180,
                 "Location coordinates must be valid");

    if (sGeoid == null)
    {
      try (InputStream input = context.getAssets().open(ASSET_PATH))
      {
        sGeoid = new Geoid(input);
      }
      catch (IOException e)
      {
        throw new IllegalStateException("Cannot load bundled geoid model", e);
      }
    }
    double geoidHeight = sGeoid.getHeight(latitude, longitude);
    LocationCompat.setMslAltitudeMeters(location, location.getAltitude() - geoidHeight);
    LocationCompat.removeMslAltitudeAccuracy(location);
    if (LocationCompat.hasVerticalAccuracy(location))
    {
      float accuracy = LocationCompat.getVerticalAccuracyMeters(location);
      if (Float.isFinite(accuracy) && accuracy >= 0)
        LocationCompat.setMslAltitudeAccuracyMeters(location, (float) Math.hypot(accuracy, RMSE_METERS));
    }
  }
}
