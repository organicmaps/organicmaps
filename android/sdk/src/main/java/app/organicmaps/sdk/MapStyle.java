package app.organicmaps.sdk;

import androidx.annotation.NonNull;

/**
 * Native map style family and light/dark appearance. Use on the main thread after framework initialization.
 */
public enum MapStyle
{
  Clear(0),
  Dark(1),
  VehicleClear(3),
  VehicleDark(4),
  OutdoorsClear(5),
  OutdoorsDark(6),
  CyclingClear(7),
  CyclingDark(8);

  MapStyle(int value)
  {
    this.value = value;
  }

  @NonNull
  public static MapStyle get()
  {
    return valueOf(nativeGet());
  }

  /** Synchronizes the selected map style with the display's appearance and navigation state. */
  public static void setForCurrentMode(boolean dark, boolean vehicleNavigation, boolean isRendererActive)
  {
    nativeSetForCurrentMode(dark, vehicleNavigation, isRendererActive);
  }

  /**
   * Converts a native style identifier to an enum value.
   * @throws IllegalArgumentException for an unknown native identifier
   */
  @NonNull
  public static MapStyle valueOf(int value)
  {
    for (MapStyle mapStyle : MapStyle.values())
    {
      if (mapStyle.value == value)
        return mapStyle;
    }
    throw new IllegalArgumentException("Unknown map style value: " + value);
  }

  private final int value;

  private static native int nativeGet();

  private static native void nativeSetForCurrentMode(boolean dark, boolean vehicleNavigation, boolean isRendererActive);
}
