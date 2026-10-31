package app.organicmaps.sdk;

import androidx.annotation.NonNull;

/**
 * Supported route profiles and their native identifiers. Route operations belong on the main thread.
 * Use {@link app.organicmaps.sdk.routing.RoutingController} to build and manage routes.
 */
public enum Router
{
  Vehicle(0),
  Pedestrian(1),
  Bicycle(2),
  Transit(3),
  Ruler(4);

  Router(int type)
  {
    this.type = type;
  }

  /**
   * Sets the native route profile; this alone does not build a route.
   */
  public static void set(@NonNull Router routerType)
  {
    nativeSet(routerType.type);
  }

  public static Router get()
  {
    return valueOf(nativeGet());
  }

  /**
   * Returns the profile saved by the native routing framework.
   */
  public static Router getLastUsed()
  {
    return valueOf(nativeGetLastUsed());
  }

  /**
   * Chooses a suggested profile for two WGS84 positions in degrees; this does not build a route.
   */
  public static Router getBest(double srcLat, double srcLon, double dstLat, double dstLon)
  {
    return valueOf(nativeGetBest(srcLat, srcLon, dstLat, dstLon));
  }

  /**
   * Converts a native profile identifier.
   * @throws IllegalArgumentException for an unknown profile identifier
   */
  public static Router valueOf(int type)
  {
    for (Router router : values())
      if (router.type == type)
        return router;
    throw new IllegalArgumentException("Unknown router: " + type);
  }

  /**
   * Returns the native profile identifier used by JNI, rather than the enum ordinal.
   */
  public int getType()
  {
    return type;
  }

  private final int type;

  private static native void nativeSet(int routerType);

  private static native int nativeGet();

  private static native int nativeGetLastUsed();

  private static native int nativeGetBest(double srcLat, double srcLon, double dstLat, double dstLon);
}
