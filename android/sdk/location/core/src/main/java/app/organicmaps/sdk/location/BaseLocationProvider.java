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
    @UiThread
    void onLocationDisabled();
    @UiThread
    void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent);
    /** Reports a Google request failure; the owner decides whether acquisition can continue. */
    @UiThread
    void onFusedLocationFailure();
    /** Reports progress from the Play-services-compatible backend, not the native system fused source. */
    @UiThread
    void onFusedLocationProgress(@NonNull StartupPhase phase);
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
