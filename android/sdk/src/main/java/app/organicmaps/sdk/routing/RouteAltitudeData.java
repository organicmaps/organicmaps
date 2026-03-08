package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;

public class RouteAltitudeData
{
  @NonNull
  public final double[] distances; // meters from start
  @NonNull
  public final int[] altitudes;    // meters elevation

  public RouteAltitudeData(@NonNull double[] distances, @NonNull int[] altitudes)
  {
    this.distances = distances;
    this.altitudes = altitudes;
  }
}
