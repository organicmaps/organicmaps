package app.organicmaps.sdk;

import androidx.annotation.NonNull;

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

  public static void set(@NonNull MapStyle mapStyle)
  {
    nativeSet(mapStyle.value);
  }

  /**
   * This method allows to set new map style without immediate applying. It can be used before
   * engine recreation instead of nativeSetMapStyle to avoid huge flow of OpenGL invocations.
   *
   * @param mapStyle style index
   */
  public static void mark(@NonNull MapStyle mapStyle)
  {
    nativeMark(mapStyle.value);
  }

  /**
   * Stores the platform's light/dark choice in the core, which combines it with the active family
   * (vehicle / outdoors / default) to resolve the concrete map style. Storing only, no apply.
   */
  public static void setNightMode(boolean nightMode)
  {
    nativeSetNightMode(nightMode);
  }

  /**
   * Resolves the map style for the current routing/outdoors mode. Both the family (vehicle / outdoors
   * / default) and the darkness come from the core; push the latter via {@link #setNightMode}.
   */
  @NonNull
  public static MapStyle resolveForMode()
  {
    return valueOf(nativeResolveForMode());
  }

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

  private static native int nativeResolveForMode();

  private static native void nativeSetNightMode(boolean nightMode);
}
