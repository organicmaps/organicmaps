package com.mapswithme.maps.bookmarks.data;

import java.util.HashMap;
import java.util.Map;

public class Metadata
{
  // values MUST correspond to definitions from feature_meta.hpp
  public enum MetadataType
  {
    FMD_CUISINE(1),
    FMD_OPEN_HOURS(2),
    FMD_PHONE_NUMBER(3),
    FMD_FAX_NUMBER(4),
    FMD_STARS(5),
    FMD_OPERATOR(6),
    FMD_URL(7),
    FMD_INTERNET(8),
    FMD_ELE(9),
    FMD_TURN_LANES(10),
    FMD_TURN_LANES_FORWARD(11),
    FMD_TURN_LANES_BACKWARD(12),
    FMD_EMAIL(13);

    private int mMetaType;

    MetadataType(int metadataType)
    {
      mMetaType = metadataType;
    }

    public static MetadataType fromInt(int metaType)
    {
      for (MetadataType type : values())
        if (type.mMetaType == metaType)
          return type;

      return null;
    }
  }

  private Map<MetadataType, String> mMetadataMap = new HashMap<>();

  /**
   * Adds metadata with type code and value. Returns false if metaType is wrong or unknown
   *
   * @param metaType
   * @param metaValue
   * @return true, if metadata was added, false otherwise
   */
  public boolean addMetadata(int metaType, String metaValue)
  {
    final MetadataType type = MetadataType.fromInt(metaType);
    if (type == null)
      return false;

    mMetadataMap.put(type, metaValue);
    return true;
  }

  /**
   * Adds metadata with type and value.
   *
   * @param type
   * @param value
   * @return true, if metadata was added, false otherwise
   */
  public boolean addMetadata(MetadataType type, String value)
  {
    mMetadataMap.put(type, value);
    return true;
  }

  /**
   * @param type
   * @return null if metadata doesn't exist
   */
  public String getMetadata(MetadataType type)
  {
    return mMetadataMap.get(type);
  }
}
