package app.organicmaps.location;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class LocationState
{
  public interface ModeChangeListener
  {
    void onMyPositionModeChanged(int newMode);
  }

  interface PendingTimeoutListener
  {
    void onLocationPendingTimeout();
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ PENDING_POSITION, NOT_FOLLOW_NO_POSITION, NOT_FOLLOW, FOLLOW, FOLLOW_AND_ROTATE})
  @interface Value {}

  // These values should correspond to location::EMyPositionMode enum (from platform/location.hpp)
  public static final int PENDING_POSITION = 0;
  public static final int NOT_FOLLOW_NO_POSITION = 1;
  public static final int NOT_FOLLOW = 2;
  public static final int FOLLOW = 3;
  public static final int FOLLOW_AND_ROTATE = 4;

  public static native void nativeSwitchToNextMode();
  @Value
  public static native int nativeGetMode();

  public static native void nativeSetListener(@NonNull ModeChangeListener listener);
  public static native void nativeRemoveListener();

  static native void nativeSetLocationPendingTimeoutListener(@NonNull PendingTimeoutListener listener);
  static native void nativeRemoveLocationPendingTimeoutListener();

  private LocationState() {}

  /**
   * Checks if location state on the map is active (so its not turned off or pending).
   */
  static boolean isTurnedOn()
  {
    return hasLocation(nativeGetMode());
  }

  static boolean hasLocation(int mode)
  {
    return (mode > NOT_FOLLOW_NO_POSITION);
  }

  static String nameOf(@Value int mode)
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
