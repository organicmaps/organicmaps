package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;

/** Receives processed measurements, or null on location loss/navigation end. Registration does not replay events. */
public interface RoutingInfoListener
{
  @Keep
  @MainThread
  void onRoutingInfoUpdated(@Nullable RoutingInfo info);
}
