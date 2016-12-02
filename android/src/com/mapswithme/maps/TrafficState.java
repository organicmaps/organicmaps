package com.mapswithme.maps;

public final class TrafficState
{
  public interface StateChangeListener
  {
    // This method is called from JNI layer.
    @SuppressWarnings("unused")
    void onTrafficStateChanged(int newMode);
  }

  // These values should correspond to
  // TrafficManager::TrafficState enum (from map/traffic_manager.hpp)
  public static final int DISABLED = 0;
  public static final int ENABLED = 1;
  public static final int WAITING_DATA = 2;
  public static final int OUTDATED = 3;
  public static final int NO_DATA = 4;
  public static final int NETWORK_ERROR = 5;
  public static final int EXPIRED_DATA = 6;
  public static final int EXPIRED_APP = 7;

  public static native void nativeSetListener(StateChangeListener listener);
  public static native void nativeRemoveListener();

  private TrafficState() {}

  public static String nameOf(int state)
  {
    switch (state)
    {
      case DISABLED:
        return "DISABLED";

      case ENABLED:
        return "ENABLED";

      case WAITING_DATA:
        return "WAITING_DATA";

      case OUTDATED:
        return "OUTDATED";

      case NO_DATA:
        return "NO_DATA";

      case NETWORK_ERROR:
        return "NETWORK_ERROR";

      case EXPIRED_DATA:
        return "EXPIRED_DATA";

      case EXPIRED_APP:
        return "EXPIRED_APP";

      default:
        return "Unknown: " + state;
    }
  }
}
