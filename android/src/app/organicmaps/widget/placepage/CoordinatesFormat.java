package app.organicmaps.widget.placepage;

public enum CoordinatesFormat
{
  LatLonDMS(0),     // Latitude, Longitude in degrees minutes seconds format, comma separated
  LatLonDecimal(1), // Latitude, Longitude in decimal format, comma separated
  OLCFull(2),       // Open location code, full format
  OSMLink(3);       // Link to the OSM. E.g. https://osm.org/go/xcXjyqQlq-?m=

  private final int id;

  CoordinatesFormat(int id)
  {
    this.id = id;
  }

  public int getId()
  {
    return id;
  }

  public static CoordinatesFormat fromId(int id)
  {
    for (CoordinatesFormat cursor: CoordinatesFormat.values())
    {
      if (cursor.id == id)
        return cursor;
    }

    return LatLonDMS; // Default format is DMS
  }
}
