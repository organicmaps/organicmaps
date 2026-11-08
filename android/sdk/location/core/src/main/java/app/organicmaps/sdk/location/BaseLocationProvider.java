package app.organicmaps.sdk.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.PendingIntent;
import android.location.Location;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.annotation.UiThread;

public abstract class BaseLocationProvider
{
  public interface Listener
  {
    @UiThread
    void onLocationChanged(@NonNull Location location);
    /** Delivers an authorized native-source handoff with its first complete fix. */
    @UiThread
    default void onLocationChanged(@NonNull Location location, boolean sourceChanged)
    {
      onLocationChanged(location);
    }
    @UiThread
    void onLocationDisabled();
    /** Reports loss of coarse and fine location access detected by the Google provider. */
    @UiThread
    void onLocationPermissionDenied();
    // Used by GoogleFusedLocationProvider.
    @SuppressWarnings("unused")
    @UiThread
    void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent);
    // Used by GoogleFusedLocationProvider.
    @SuppressWarnings("unused")
    @UiThread
    void onFusedLocationUnsupported();
  }

  @NonNull
  protected final Listener mListener;

  protected BaseLocationProvider(@NonNull Listener listener)
  {
    mListener = listener;
  }

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  protected abstract void start(long interval);
  protected abstract void stop();
}
