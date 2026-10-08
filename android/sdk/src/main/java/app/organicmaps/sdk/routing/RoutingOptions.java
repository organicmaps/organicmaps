package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.settings.RoadType;

public final class RoutingOptions
{
  private static final int ROAD_TYPES_MASK = (1 << RoadType.values().length) - 1;

  public static void addOption(@NonNull RoadType roadType)
  {
    nativeAddOption(roadType.ordinal());
  }

  public static void removeOption(@NonNull RoadType roadType)
  {
    nativeRemoveOption(roadType.ordinal());
  }

  public static boolean hasOption(@NonNull RoadType roadType)
  {
    return (getOptions() & (1 << roadType.ordinal())) != 0;
  }

  public static boolean hasAnyOptions()
  {
    return getOptions() != 0;
  }

  public static boolean isRouteOptimizationEnabled()
  {
    return nativeIsRouteOptimizationEnabled();
  }

  public static void setRouteOptimizationEnabled(boolean enabled)
  {
    nativeSetRouteOptimizationEnabled(enabled);
  }

  public static int getOptions()
  {
    // Only expose bits represented by RoadType; the settings badge counts those same options.
    return nativeGetOptions() & ROAD_TYPES_MASK;
  }

  private RoutingOptions() throws IllegalAccessException
  {
    throw new IllegalAccessException("RoutingOptions is a utility class and should not be instantiated");
  }
  private static native void nativeAddOption(int option);

  private static native void nativeRemoveOption(int option);

  private static native int nativeGetOptions();

  private static native boolean nativeIsRouteOptimizationEnabled();

  private static native void nativeSetRouteOptimizationEnabled(boolean enabled);
}
