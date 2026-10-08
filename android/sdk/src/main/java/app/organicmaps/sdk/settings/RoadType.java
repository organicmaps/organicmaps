package app.organicmaps.sdk.settings;

public enum RoadType
{
  Usual(1),
  Toll(2),
  Motorway(4),
  Ferry(8),
  Dirty(16);

  public final int nativeValue;

  RoadType(int nativeValue)
  {
    this.nativeValue = nativeValue;
  }

  public static RoadType fromNativeValue(int value)
  {
    for (RoadType roadType : values())
      if (roadType.nativeValue == value)
        return roadType;
    throw new IllegalArgumentException("Unknown road type: " + value);
  }
}
