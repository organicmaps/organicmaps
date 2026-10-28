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
  OutdoorsDark(6);

  MapStyle(int value)
  {
    this.value = value;
  }

  @NonNull
  public static MapStyle get()
  {
    return valueOf(nativeGet());
  }

  /**
   * Applies a map style to an existing renderer; this can recreate rendering resources.
   * Use {@link #mark} before the renderer is first created.
   */
  public static void set(@NonNull MapStyle mapStyle)
  {
    nativeSet(mapStyle.value);
  }

  /**
   * Records the map style to use when the renderer is created without immediately rebuilding graphics resources.
   * @param mapStyle desired style
   */
  public static void mark(@NonNull MapStyle mapStyle)
  {
    nativeMark(mapStyle.value);
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

  private static native void nativeSet(int mapStyle);

  private static native int nativeGet();

  private static native void nativeMark(int mapStyle);
}
