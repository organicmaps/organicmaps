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
    @UiThread
    void onLocationDisabled();
    @UiThread
    void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent);
    /** Reports a Google request failure; the owner decides whether acquisition can continue. */
    @UiThread
    void onFusedLocationFailure();
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
