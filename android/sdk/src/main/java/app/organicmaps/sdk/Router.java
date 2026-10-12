package app.organicmaps.sdk;

import androidx.annotation.NonNull;

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

  // The app is for driving only, other routers are never used.
  public static void set(@NonNull Router routerType)
  {
    nativeSet(Vehicle.type);
  }

  public static Router get()
  {
    return valueOf(nativeGet());
  }

  public static Router getLastUsed()
  {
    return Vehicle;
  }

  public static Router getBest(double srcLat, double srcLon, double dstLat, double dstLon)
  {
    return Vehicle;
  }

  public static Router valueOf(int type)
  {
    return Router.values()[type];
  }

  private final int type;

  private static native void nativeSet(int routerType);

  private static native int nativeGet();
}
