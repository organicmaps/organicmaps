package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.settings.RoadType;
import java.util.EnumSet;
import java.util.Set;

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
    return (nativeGetOptions(router.getType()) & roadType.nativeValue) != 0;
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
    return nativeGetOptions(router.getType()) != 0;
  }

  public static boolean isRouteOptimizationEnabled()
  {
    return nativeIsRouteOptimizationEnabled();
  }

  public static void setRouteOptimizationEnabled(boolean enabled)
  {
    nativeSetRouteOptimizationEnabled(enabled);
  }

  @NonNull
  public static Set<RoadType> getActiveRoadTypes(@NonNull Router router)
  {
    Set<RoadType> roadTypes = EnumSet.noneOf(RoadType.class);
    final int mask = nativeGetOptions(router.getType());
    for (RoadType each : RoadType.values())
      if ((mask & each.nativeValue) != 0)
        roadTypes.add(each);
    return roadTypes;
  }

  private RoutingOptions() {}

  private static native int nativeGetOptions(int routerType);

  private static native int nativeGetSupportedOptions(int routerType);

  private static native void nativeAddOption(int routerType, int option);

  private static native void nativeRemoveOption(int routerType, int option);

  private static native boolean nativeIsRouteOptimizationEnabled();

  private static native void nativeSetRouteOptimizationEnabled(boolean enabled);
}
