package com.mapswithme.maps.routing;

import android.support.annotation.NonNull;

import com.mapswithme.maps.settings.RoadType;

public class RoutingOptions
{
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
    return nativeHasOption(roadType.ordinal());
  }

  private static native void nativeAddOption(int option);
  private static native void nativeRemoveOption(int option);
  private static native boolean nativeHasOption(int option);
}
