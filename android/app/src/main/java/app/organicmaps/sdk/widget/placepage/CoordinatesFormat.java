package app.organicmaps.sdk.widget.placepage;

public enum CoordinatesFormat
{
  LatLonDMS(0, "DMS", false), // Latitude, Longitude in degrees minutes seconds format, comma separated
  LatLonDecimal(1, "Decimal", false), // Latitude, Longitude in decimal format, comma separated
  OLCFull(2, "OLC", false), // Open location code, full format
  OSMLink(3, "osm.org", false), // Link to the OSM. E.g. https://osm.org/go/xcXjyqQlq-?m=
  UTM(4, "UTM", true), // Universal Transverse Mercator
  MGRS(5, "MGRS", true); // Military Grid Reference System

  private final int id;
  private final String label;
  private final boolean showLabel;

  CoordinatesFormat(int id, String label, boolean showLabel)
  {
    this.id = id;
    this.label = label;
    this.showLabel = showLabel;
  }

  public int getId()
  {
    return id;
  }

  public String getLabel()
  {
    return label;
  }

  public boolean showLabel()
  {
    return showLabel;
  }

  public static CoordinatesFormat fromId(int id)
  {
    for (CoordinatesFormat cursor : CoordinatesFormat.values())
    {
      if (cursor.id == id)
        return cursor;
    }

    return LatLonDMS; // Default format is DMS
  }
}
