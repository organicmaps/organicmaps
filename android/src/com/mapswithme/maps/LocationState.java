package com.mapswithme.maps;

public final class LocationState
{
  public interface ModeChangeListener
  {
    @SuppressWarnings("unused")
    void onMyPositionModeChanged(int newMode);
  }

  // These values should correspond to location::EMyPositionMode enum (from platform/location.hpp)
  public static final int PENDING_POSITION = 0;
  public static final int NOT_FOLLOW_NO_POSITION = 1;
  public static final int NOT_FOLLOW = 2;
  public static final int FOLLOW = 3;
  public static final int FOLLOW_AND_ROTATE = 4;

  public static native void nativeSwitchToNextMode();
  private static native int nativeGetMode();

  public static native void nativeSetListener(ModeChangeListener listener);
  public static native void nativeRemoveListener();

  private LocationState() {}

  /**
   * Checks if location state on the map is active (so its not turned off or pending).
   */
  public static boolean isTurnedOn()
  {
    return hasLocation(getMode());
  }

  public static boolean hasLocation(int mode)
  {
    return (mode > NOT_FOLLOW_NO_POSITION);
  }

  public static int getMode()
  {
    MwmApplication.get().initNativeCore();
    return nativeGetMode();
  }

  public static String nameOf(int mode)
  {
    switch (mode)
    {
    case PENDING_POSITION:
      return "PENDING_POSITION";

    case NOT_FOLLOW_NO_POSITION:
      return "NOT_FOLLOW_NO_POSITION";

    case NOT_FOLLOW:
      return "NOT_FOLLOW";

    case FOLLOW:
      return "FOLLOW";

    case FOLLOW_AND_ROTATE:
      return "FOLLOW_AND_ROTATE";

    default:
      return "Unknown: " + mode;
    }
  }
}
