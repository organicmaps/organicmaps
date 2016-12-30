package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

// TODO(yunikkk): Refactor. Displayed information is different from edited information, and it's better to
// separate them. Simple getters from jni place_page::Info and osm::EditableFeature should be enough.
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

  protected String mTitle;
  private String mSubtitle;
  private double mLat;
  private double mLon;
  private String mAddress;
  private Metadata mMetadata;
  private String mApiId;
  @Nullable
  private Banner mBanner;
  private boolean mReachableByTaxi;

  public MapObject(@MapObjectType int mapObjectType, String title, String subtitle, String address,
                   double lat, double lon, String apiId, @Nullable Banner banner,
                   boolean reachableByTaxi)
  {
    this(mapObjectType, title, subtitle, address, lat, lon, new Metadata(), apiId, banner,
         reachableByTaxi);
  }

  public MapObject(@MapObjectType int mapObjectType, String title, String subtitle, String address,
                   double lat, double lon, Metadata metadata, String apiId, @Nullable Banner banner,
                   boolean reachableByTaxi)
  {
    mMapObjectType = mapObjectType;
    mTitle = title;
    mSubtitle = subtitle;
    mAddress = address;
    mLat = lat;
    mLon = lon;
    mMetadata = metadata;
    mApiId = apiId;
    mBanner = banner;
    mReachableByTaxi = reachableByTaxi;
  }

  protected MapObject(Parcel source)
  {
    //noinspection ResourceType
    this(source.readInt(),    // MapObjectType
         source.readString(), // Title
         source.readString(), // Subtitle
         source.readString(), // Address
         source.readDouble(), // Lat
         source.readDouble(), // Lon
         (Metadata) source.readParcelable(Metadata.class.getClassLoader()),
         source.readString(), // ApiId;
         (Banner) source.readParcelable(Banner.class.getClassLoader()),
         source.readByte() != 0); // ReachableByTaxi
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
           TextUtils.equals(mTitle, other.mTitle) &&
           TextUtils.equals(mSubtitle, other.mSubtitle);
  }

  public static boolean same(MapObject one, MapObject another)
  {
    //noinspection SimplifiableIfStatement
    if (one == null && another == null)
      return true;

    return (one != null && one.sameAs(another));
  }

  public double getScale() { return 0; }

  public String getTitle() { return mTitle; }

  public String getSubtitle() { return mSubtitle; }

  public double getLat() { return mLat; }

  public double getLon() { return mLon; }

  public String getAddress() { return mAddress; }

  @NonNull
  public String getMetadata(Metadata.MetadataType type)
  {
    final String res = mMetadata.getMetadata(type);
    return res == null ? "" : res;
  }

  @MapObjectType
  public int getMapObjectType()
  {
    return mMapObjectType;
  }

  public String getApiId()
  {
    return mApiId;
  }

  @Nullable
  public Banner getBanner()
  {
    return mBanner;
  }

  public boolean isReachableByTaxi()
  {
    return mReachableByTaxi;
  }

  public void setLat(double lat)
  {
    mLat = lat;
  }

  public void setLon(double lon)
  {
    mLon = lon;
  }

  public void setSubtitle(String typeName)
  {
    mSubtitle = typeName;
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

  public boolean hasPhoneNumber()
  {
    return !TextUtils.isEmpty(getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
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
    dest.writeString(mTitle);
    dest.writeString(mSubtitle);
    dest.writeString(mAddress);
    dest.writeDouble(mLat);
    dest.writeDouble(mLon);
    dest.writeParcelable(mMetadata, 0);
    dest.writeString(mApiId);
    dest.writeParcelable(mBanner, 0);
    dest.writeByte((byte) (mReachableByTaxi ? 1 : 0));
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
