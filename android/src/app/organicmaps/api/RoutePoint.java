package app.organicmaps.api;

/**
 * Represents url_scheme::RoutePoint from core.
 */
public class RoutePoint
{
  public final double mLat;
  public final double mLon;
  public final String mName;

  public RoutePoint(double lat, double lon, String name)
  {
    mLat = lat;
    mLon = lon;
    mName = name;
  }
}
