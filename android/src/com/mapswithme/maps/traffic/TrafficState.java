package com.mapswithme.maps.traffic;

import android.support.annotation.IntDef;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class TrafficState
{
  interface StateChangeListener
  {
    // This method is called from JNI layer.
    @SuppressWarnings("unused")
    @MainThread
    void onTrafficStateChanged(@Value int state);
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ DISABLED, ENABLED, WAITING_DATA, OUTDATED, NO_DATA, NETWORK_ERROR, EXPIRED_DATA, EXPIRED_APP})

  @interface Value {}

  // These values should correspond to
  // TrafficManager::TrafficState enum (from map/traffic_manager.hpp)
  static final int DISABLED = 0;
  static final int ENABLED = 1;
  static final int WAITING_DATA = 2;
  static final int OUTDATED = 3;
  static final int NO_DATA = 4;
  static final int NETWORK_ERROR = 5;
  static final int EXPIRED_DATA = 6;
  static final int EXPIRED_APP = 7;

  @MainThread
  static native void nativeSetListener(@NonNull StateChangeListener listener);
  static native void nativeRemoveListener();
  static native void nativeEnable();
  static native void nativeDisable();
  static native boolean nativeIsEnabled();

  private TrafficState() {}

  static String nameOf(int state)
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
