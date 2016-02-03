package com.mapswithme.maps.bookmarks.data;

import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public class MapObject implements Parcelable
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({POI, API_POINT, BOOKMARK, MY_POSITION, SEARCH})
  public @interface MapObjectType {}

  public static final int POI = 0;
  public static final int API_POINT = 1;
  public static final int BOOKMARK = 2;
  public static final int MY_POSITION = 3;
  public static final int SEARCH = 4;

  @MapObjectType protected final int mMapObjectType;

  protected String mName;
  protected double mLat;
  protected double mLon;
  protected String mTypeName;
  protected String mStreet;
  protected String mHouseNumber;
  protected Metadata mMetadata;
  protected boolean mIsDroppedPin;
  protected String mSearchId;

  // TODO @yunikkk add static factory methods for different mapobject creation

  public MapObject(@MapObjectType int mapObjectType, String name, double lat, double lon, String typeName, String street, String house)
  {
    this(mapObjectType, name, lat, lon, typeName, street, house, new Metadata());
  }

  public MapObject(@MapObjectType int mapObjectType, String name, double lat, double lon, String typeName, String street, String house, Metadata metadata)
  {
    mMapObjectType = mapObjectType;
    mName = name;
    mLat = lat;
    mLon = lon;
    mTypeName = typeName;
    mStreet = street;
    mHouseNumber = house;
    mMetadata = metadata;
    mIsDroppedPin = TextUtils.isEmpty(mName);
  }

  protected MapObject(Parcel source)
  {
    //noinspection ResourceType
    this(source.readInt(),    // MapObjectType
         source.readString(), // Name
         source.readDouble(), // Lat
         source.readDouble(), // Lon
         source.readString(), // TypeName
         source.readString(), // Street
         source.readString(), // HouseNumber
         (Metadata) source.readParcelable(Metadata.class.getClassLoader()));

    mIsDroppedPin = source.readByte() != 0;
    mSearchId = source.readString();
  }

  public void setDefaultIfEmpty()
  {
    if (TextUtils.isEmpty(mName))
      mName = TextUtils.isEmpty(mTypeName) ? MwmApplication.get().getString(R.string.dropped_pin)
                                           : mTypeName;

    if (TextUtils.isEmpty(mTypeName))
      mTypeName = MwmApplication.get().getString(R.string.placepage_unsorted);
  }

  /**
   * If you override {@link #equals(Object)} it is also required to override {@link #hashCode()}.
   * MapObject does not participate in any sets or other collections that need {@code hashCode()}.
   * So {@code sameAs()} serves as {@code equals()} but does not break the equals+hashCode contract.
   */
  public boolean sameAs(MapObject other)
  {
    if (other == null)
      return false;

    if (this == other)
      return true;

    //noinspection SimplifiableIfStatement
    if (getClass() != other.getClass())
      return false;

    return Double.doubleToLongBits(mLon) == Double.doubleToLongBits(other.mLon) &&
           Double.doubleToLongBits(mLat) == Double.doubleToLongBits(other.mLat) &&
           TextUtils.equals(mName, other.mName) &&
           TextUtils.equals(mTypeName, other.mTypeName);
  }

  public static boolean same(MapObject one, MapObject another)
  {
    //noinspection SimplifiableIfStatement
    if (one == null && another == null)
      return true;

    return (one != null && one.sameAs(another));
  }

  public double getScale() { return 0; }

  public String getName() { return mName; }

  public double getLat() { return mLat; }

  public double getLon() { return mLon; }

  public String getTypeName() { return mTypeName; }

  public boolean getIsDroppedPin()
  {
    return mIsDroppedPin;
  }

  @NonNull
  public String getMetadata(Metadata.MetadataType type)
  {
    final String res = mMetadata.getMetadata(type);
    return res == null ? "" : res;
  }

  /**
   * @return properly formatted and translated cuisine string.
   */
  @NonNull
  public String getFormattedCuisine()
  {
    final String rawCuisines = mMetadata.getMetadata(Metadata.MetadataType.FMD_CUISINE);
    if (TextUtils.isEmpty(rawCuisines))
      return "";

    final StringBuilder result = new StringBuilder();
    // search translations for each cuisine
    final Resources resources = MwmApplication.get().getResources();
    for (String rawCuisine : Metadata.splitCuisines(rawCuisines))
    {
      int resId = resources.getIdentifier(Metadata.osmCuisineToStringName(Metadata.normalizeCuisine(rawCuisine)), "string", BuildConfig.APPLICATION_ID);
      if (result.length() > 0)
        result.append(", ");
      result.append(resId == 0 ? rawCuisine : resources.getString(resId));
    }

    return result.toString();
  }

  public String getStreet()
  {
    return mStreet;
  }

  public String getHouseNumber()
  {
    return mHouseNumber;
  }

  @MapObjectType
  public int getMapObjectType()
  {
    return mMapObjectType;
  }

  public String getSearchId()
  {
    return mSearchId;
  }

  public void setName(String name)
  {
    mName = name;
  }

  public void setHouseNumber(String houseNumber)
  {
    mHouseNumber = houseNumber;
  }

  public void setLat(double lat)
  {
    mLat = lat;
  }

  public void setLon(double lon)
  {
    mLon = lon;
  }

  public void setTypeName(String typeName)
  {
    mTypeName = typeName;
  }

  public void addMetadata(Metadata.MetadataType type, String value)
  {
    mMetadata.addMetadata(type.toInt(), value);
  }

  public void addMetadata(int type, String value)
  {
    mMetadata.addMetadata(type, value);
  }

  public void addMetadata(int[] types, String[] values)
  {
    for (int i = 0; i < types.length; i++)
      addMetadata(types[i], values[i]);
  }

  public void setStreet(String street)
  {
    mStreet = street;
  }

  public static boolean isOfType(@MapObjectType int type, MapObject object)
  {
    return object != null && object.getMapObjectType() == type;
  }

  protected static MapObject readFromParcel(Parcel source)
  {
    @MapObjectType int type = source.readInt();
    if (type == BOOKMARK)
      return new Bookmark(source);

    return new MapObject(source);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mMapObjectType); // write map object type twice - first int is used to distinguish created object (MapObject or Bookmark)
    dest.writeInt(mMapObjectType);
    dest.writeString(mName);
    dest.writeDouble(mLat);
    dest.writeDouble(mLon);
    dest.writeString(mTypeName);
    dest.writeString(mStreet);
    dest.writeString(mHouseNumber);
    dest.writeParcelable(mMetadata, 0);
    dest.writeByte((byte) (mIsDroppedPin ? 1 : 0));
    dest.writeString(mSearchId);
  }

  public static final Creator<MapObject> CREATOR = new Creator<MapObject>()
  {
    @Override
    public MapObject createFromParcel(Parcel source)
    {
      return readFromParcel(source);
    }

    @Override
    public MapObject[] newArray(int size)
    {
      return new MapObject[size];
    }
  };
}
