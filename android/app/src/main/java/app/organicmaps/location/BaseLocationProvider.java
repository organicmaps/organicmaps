package app.organicmaps.location;

import android.app.PendingIntent;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

abstract class BaseLocationProvider
{
  interface Listener
  {
    @UiThread
    void onLocationChanged(@NonNull Location location);
    @UiThread
    void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent);
    @UiThread
    void onLocationDisabled();
    @UiThread
    void onFusedLocationUnsupported();
  }

  @NonNull
  protected Listener mListener;

  protected BaseLocationProvider(@NonNull Listener listener)
  {
    mListener = listener;
  }

  protected abstract void start(long interval);
  protected abstract void stop();
}
