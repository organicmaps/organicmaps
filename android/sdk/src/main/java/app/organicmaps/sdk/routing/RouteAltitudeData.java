package app.organicmaps.sdk.routing;

import androidx.annotation.NonNull;

public class RouteAltitudeData
{
  @NonNull
  public final double[] distances; // meters from start
  @NonNull
  public final int[] altitudes; // meters elevation
  @NonNull
  public final double[] lats;
  @NonNull
  public final double[] lons;

  public RouteAltitudeData(@NonNull double[] distances, @NonNull int[] altitudes, @NonNull double[] lats,
                           @NonNull double[] lons)
  {
    this.distances = distances;
    this.altitudes = altitudes;
    this.lats = lats;
    this.lons = lons;
  }
}
