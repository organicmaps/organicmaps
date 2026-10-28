package app.organicmaps.sdk.location;

import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.Map;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Process-wide position-display mode owned by the native renderer.
 * Reading or changing the mode requires an existing rendering engine. A listener can be registered after framework
 * initialization, before the renderer exists; registration does not replay the current mode. Use on the main thread.
 * The host requests runtime permissions and starts or stops {@link LocationHelper} when the listener is notified.
 */
public final class LocationState
{
  public static final String LOCATION_TAG = LocationState.class.getSimpleName();

  /**
   * Receives changes to the native position-display mode.
   */
  public interface ModeChangeListener
  {
    /**
     * Called when the mode changes. Update position UI and location polling for {@code newMode}.
     */
    @Keep
    @SuppressWarnings("unused")
    void onMyPositionModeChanged(int newMode);
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({PENDING_POSITION, NOT_FOLLOW_NO_POSITION, NOT_FOLLOW, FOLLOW, FOLLOW_AND_ROTATE})
  @interface Value
  {}

  // These values should correspond to location::EMyPositionMode enum (from platform/location.hpp)
  public static final int PENDING_POSITION = 0;
  public static final int NOT_FOLLOW_NO_POSITION = 1;
  public static final int NOT_FOLLOW = 2;
  public static final int FOLLOW = 3;
  public static final int FOLLOW_AND_ROTATE = 4;

  // These constants should correspond to values defined in platform/location.hpp
  // Leave 0-value as no any error.
  // private static final int ERROR_UNKNOWN = 0;
  // private static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;
  // public static final int ERROR_TIMEOUT = 4; // Unused on Android (only used on Qt)

  /**
   * Advances the position button state machine; reports the resulting mode to the registered listener.
   */
  public static native void nativeSwitchToNextMode();
  @Value
  private static native int nativeGetMode();

  /**
   * Replaces the single position-mode listener; unregister it when its host ends. Registration can precede renderer
   * creation and does not replay the current mode.
   */
  public static native void nativeSetListener(@NonNull ModeChangeListener listener);
  /**
   * Removes the single position-mode listener.
   */
  public static native void nativeRemoveListener();

  /**
   * Reports a denied location permission or disabled location provider using an {@code ERROR_*} constant.
   */
  public static native void nativeOnLocationError(int errorCode);

  static native void nativeLocationUpdated(long time, double lat, double lon, float accuracyH, double altitude,
                                           float accuracyV, float speed, float bearing);

  private LocationState() {}

  /**
   * Returns the current {@code PENDING_POSITION}, {@code NOT_FOLLOW_*}, or {@code FOLLOW*} mode.
   * @throws IllegalStateException if the native rendering engine does not exist
   */
  @Value
  public static int getMode()
  {
    if (!Map.isEngineCreated())
      throw new IllegalStateException("Location mode is undefined until engine is created");
    return nativeGetMode();
  }

  /**
   * Returns a diagnostic name for a position-display mode; it is not localized UI text.
   */
  public static String nameOf(@Value int mode)
  {
    return switch (mode)
    {
      case PENDING_POSITION -> "PENDING_POSITION";
      case NOT_FOLLOW_NO_POSITION -> "NOT_FOLLOW_NO_POSITION";
      case NOT_FOLLOW -> "NOT_FOLLOW";
      case FOLLOW -> "FOLLOW";
      case FOLLOW_AND_ROTATE -> "FOLLOW_AND_ROTATE";
      default -> "Unknown: " + mode;
    };
  }
}
