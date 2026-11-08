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
  /** Startup progress from a Play-services-compatible location provider. */
  public enum StartupPhase
  {
    CHECKING_SETTINGS,
    REGISTERING_UPDATES,
    REGISTERED
  }

  public interface Listener
  {
    @UiThread
    void onLocationChanged(@NonNull Location location);
    /**
     * Delivers a selected native stream. A handoff authorizes this complete fix independently of the previous
     * source's quality and resets prediction; listener wrappers must preserve the flag.
     */
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
    /** Reports progress from the Play-services-compatible backend, not the native system fused source. */
    @UiThread
    default void onFusedLocationProgress(@NonNull StartupPhase phase)
    {}
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

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  protected void updateInterval(long interval)
  {
    stop();
    start(interval);
  }

  /** Rechecks deadlines and availability after returning from suspend. */
  @UiThread
  protected void onForeground()
  {}
}
