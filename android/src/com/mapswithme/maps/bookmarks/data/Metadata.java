package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.HashMap;
import java.util.Map;

public class Metadata implements Parcelable
{
  // Values must correspond to definitions from feature_meta.hpp.
  public enum MetadataType
  {
    FMD_CUISINE(1),
    FMD_OPEN_HOURS(2),
    FMD_PHONE_NUMBER(3),
    FMD_FAX_NUMBER(4),
    FMD_STARS(5),
    FMD_OPERATOR(6),
    FMD_URL(7),
    FMD_WEBSITE(8),
    FMD_INTERNET(9),
    FMD_ELE(10),
    FMD_TURN_LANES(11),
    FMD_TURN_LANES_FORWARD(12),
    FMD_TURN_LANES_BACKWARD(13),
    FMD_EMAIL(14),
    FMD_POSTCODE(15),
    // TODO: It is hacked in jni and returns full Wikipedia url. Should use separate getter instead.
    FMD_WIKIPEDIA(16),
    FMD_MAXSPEED(17),
    FMD_FLATS(18);

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

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mMetadataMap.size());
    for (Map.Entry<MetadataType, String> metaEntry : mMetadataMap.entrySet())
    {
      dest.writeInt(metaEntry.getKey().mMetaType);
      dest.writeString(metaEntry.getValue());
    }
  }

  public static Metadata readFromParcel(Parcel source)
  {
    final Metadata metadata = new Metadata();
    final int size = source.readInt();
    for (int i = 0; i < size; i++)
      metadata.addMetadata(source.readInt(), source.readString());
    return metadata;
  }

  public static final Creator<Metadata> CREATOR = new Creator<Metadata>()
  {
    @Override
    public Metadata createFromParcel(Parcel source)
    {
      return readFromParcel(source);
    }

    @Override
    public Metadata[] newArray(int size)
    {
      return new Metadata[size];
    }
  };
}
