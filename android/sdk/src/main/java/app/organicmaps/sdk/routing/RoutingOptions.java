package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.settings.RoadType;

public final class RoutingOptions
{
  public static void addOption(@NonNull Router router, @NonNull RoadType roadType)
  {
    nativeAddOption(router.getType(), roadType.nativeValue);
  }

  public static void removeOption(@NonNull Router router, @NonNull RoadType roadType)
  {
    nativeRemoveOption(router.getType(), roadType.nativeValue);
  }

  public static boolean hasOption(@NonNull Router router, @NonNull RoadType roadType)
  {
    return (getOptions(router) & roadType.nativeValue) != 0;
  }

  public static boolean supportsOption(@NonNull Router router, @NonNull RoadType roadType)
  {
    return (nativeGetSupportedOptions(router.getType()) & roadType.nativeValue) != 0;
  }

  public static boolean hasSupportedOptions(@NonNull Router router)
  {
    return nativeGetSupportedOptions(router.getType()) != 0;
  }

  public static boolean hasSettings(@NonNull Router router)
  {
    // Transit still offers global route optimization; Ruler preserves the measured point order.
    return router != Router.Ruler;
  }

  public static boolean hasAnyOptions(@NonNull Router router)
  {
    return getOptions(router) != 0;
  }

  public static boolean isRouteOptimizationEnabled()
  {
    return nativeIsRouteOptimizationEnabled();
  }

  public static void setRouteOptimizationEnabled(boolean enabled)
  {
    nativeSetRouteOptimizationEnabled(enabled);
  }

  public static int getOptions(@NonNull Router router)
  {
    // Native settings expose only the options supported by this router.
    return nativeGetOptions(router.getType());
  }

  private RoutingOptions() {}

  private static native int nativeGetOptions(int routerType);

  private static native int nativeGetSupportedOptions(int routerType);

  private static native void nativeAddOption(int routerType, int option);

  private static native void nativeRemoveOption(int routerType, int option);

  private static native boolean nativeIsRouteOptimizationEnabled();

  private static native void nativeSetRouteOptimizationEnabled(boolean enabled);
}
