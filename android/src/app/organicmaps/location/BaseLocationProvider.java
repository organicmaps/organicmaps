package app.organicmaps.location;

import android.app.PendingIntent;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;

abstract class BaseLocationProvider
{
  interface Listener
  {
    @UiThread
    void onLocationChanged(@NonNull Location location);
    @UiThread
    void onLocationResolutionRequired(@Nullable PendingIntent pendingIntent);
    @UiThread
    void onLocationDisabled();
    @UiThread
    void onLocationUnsupported();
  }

  @NonNull
  protected Listener mListener;

  protected BaseLocationProvider(@NonNull Listener listener)
  {
    mListener = listener;
  }

  protected abstract void start(long interval);
  protected abstract void stop();
  protected boolean trustFusedLocations() { return false; }
}
