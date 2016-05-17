package com.mapswithme.maps;

public enum LocationState
{
  INSTANCE;

  public interface ModeChangeListener
  {
    @SuppressWarnings("unused")
    void onMyPositionModeChangedCallback(final int newMode, final boolean routingActive);
  }

  // These values should correspond to location::EMyPositionMode enum (from platform/location.hpp)
  public static final int PENDING_POSITION = 0;
  public static final int NOT_FOLLOW_NO_POSITION = 1;
  public static final int NOT_FOLLOW = 2;
  public static final int FOLLOW = 3;
  public static final int FOLLOW_AND_ROTATE = 4;

  public native void nativeSwitchToNextMode();
  private native int nativeGetMode();

  public native void nativeSetListener(ModeChangeListener listener);
  public native void nativeRemoveListener();

  /**
   * Checks if location state on the map is active (so its not turned off or pending).
   */
  public static boolean isTurnedOn()
  {
    return getMode() > NOT_FOLLOW_NO_POSITION;
  }

  public static int getMode()
  {
    MwmApplication.get().initNativeCore();
    return INSTANCE.nativeGetMode();
  }
}
