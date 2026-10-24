package app.organicmaps.sdk.location;

import androidx.annotation.Keep;
import androidx.annotation.MainThread;

public interface RouteSimulationListener
{
  // Called from JNI. Speed and bearing are negative when unknown.
  @Keep
  @SuppressWarnings("unused")
  @MainThread
  void onRouteSimulationLocation(long timeMs, double lat, double lon, float accuracy, float speed, float bearing);
}
