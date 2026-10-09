package app.organicmaps.sdk.bookmarks.data;

import android.net.Uri;
import android.os.Parcel;
import android.text.TextUtils;
import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.core.os.ParcelCompat;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

// TODO(yunikkk): Refactor. Displayed information is different from edited information, and it's better to
// separate them. Simple getters from jni place_page::Info and osm::EditableFeature should be enough.
// Used from JNI.
@Keep
public class MapObject implements PlacePageData
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({POI, API_POINT, BOOKMARK, MY_POSITION, SEARCH, TRACK, TRACK_RECORDING})
  public @interface MapObjectType
  {}

  public static final int POI = 0;
  public static final int API_POINT = 1;
  public static final int BOOKMARK = 2;
  public static final int MY_POSITION = 3;
  public static final int SEARCH = 4;
  public static final int TRACK = 5;
  public static final int TRACK_RECORDING = 6;

  private static final String kHttp = "http://";
  private static final String kHttps = "https://";

  @MapObjectType
  private final int mMapObjectType;

  @NonNull
  private String mTitle;
  @Nullable
  private final String mSecondaryTitle;
  @NonNull
  private final String mSubtitle;
  @NonNull
  private final String mAddress;
  private double mLat;
  private double mLon;

  @NonNull
  private final Map<Class<? extends MapObjectData>, MapObjectData> mData = new HashMap<>();

  protected MapObject(@MapObjectType int mapObjectType, @NonNull String title, @Nullable String secondaryTitle,
                      @NonNull String subtitle, @NonNull String address, double lat, double lon)
  {
    mMapObjectType = mapObjectType;
    mTitle = title;
    mSecondaryTitle = secondaryTitle;
    mSubtitle = subtitle;
    mAddress = address;
    mLat = lat;
    mLon = lon;
  }

  // Also called from Bookmark constructor.
  protected MapObject(@MapObjectType int type, @NonNull Parcel source)
  {
    // Type has already been read in readFromParcel method.
    mMapObjectType = type;
    // Reading order must be the same as writing order in writeToParcel.
    mTitle = Objects.requireNonNull(source.readString(), "Title cannot be null");
    mSecondaryTitle = source.readString();
    mSubtitle = Objects.requireNonNull(source.readString(), "Subtitle cannot be null");
    mAddress = Objects.requireNonNull(source.readString(), "Address cannot be null");
    mLat = source.readDouble();
    mLon = source.readDouble();
    final int typesSize = source.readInt();
    for (int i = 0; i < typesSize; i++)
    {
      final MapObjectData dataType =
          ParcelCompat.readParcelable(source, MapObjectData.class.getClassLoader(), MapObjectData.class);
      if (dataType != null)
        put(dataType);
    }
  }

  @NonNull
  public static MapObject createMapObject(@MapObjectType int mapObjectType, @NonNull String title,
                                          @NonNull String subtitle, double lat, double lon)
  {
    return new MapObject(mapObjectType, title, null, subtitle, "", lat, lon);
  }

  /**
   * If you override {@link #equals(Object)} it is also required to override {@link #hashCode()}.
   * MapObject does not participate in any sets or other collections that need {@code hashCode()}.
   * So {@code sameAs()} serves as {@code equals()} but does not break the equals+hashCode contract.
   */
  public boolean sameAs(@Nullable MapObject other)
  {
    if (other == null)
      return false;

    if (this == other)
      return true;

    if (getClass() != other.getClass())
      return false;

    return mMapObjectType == other.mMapObjectType && mTitle.equals(other.mTitle) && mSubtitle.equals(other.mSubtitle)
 && Double.doubleToLongBits(mLon) == Double.doubleToLongBits(other.mLon)
 && Double.doubleToLongBits(mLat) == Double.doubleToLongBits(other.mLat);
  }

  public static boolean same(@Nullable MapObject one, @Nullable MapObject another)
  {
    // noinspection SimplifiableIfStatement
    if (one == null && another == null)
      return true;

    return (one != null && one.sameAs(another));
  }

  public double getScale()
  {
    return 0;
  }

  @NonNull
  public String getTitle()
  {
    return mTitle;
  }

  public void setTitle(@NonNull String title)
  {
    mTitle = title;
  }

  @NonNull
  public String getName()
  {
    return getTitle();
  }

  @Nullable
  public String getSecondaryTitle()
  {
    return mSecondaryTitle;
  }

  @NonNull
  public String getSubtitle()
  {
    return mSubtitle;
  }

  public double getLat()
  {
    return mLat;
  }

  public double getLon()
  {
    return mLon;
  }

  @NonNull
  public String getAddress()
  {
    return mAddress;
  }

  @NonNull
  public String getWebsiteUrl(boolean strip, @NonNull Metadata.MetadataType type)
  {
    if (!has(Metadata.class))
      return "";
    final Metadata metadata = get(Metadata.class);
    if (!metadata.has(type))
      return "";
    final String website = Uri.decode(metadata.get(type));
    final int len = website.length();
    if (strip && len > 1)
    {
      final int start = website.startsWith(kHttps) ? kHttps.length() : (website.startsWith(kHttp) ? kHttp.length() : 0);
      final int end = website.endsWith("/") ? len - 1 : len;
      return website.substring(start, end);
    }
    return website;
  }

  public void setLat(double lat)
  {
    mLat = lat;
  }

  public void setLon(double lon)
  {
    mLon = lon;
  }

  public boolean hasPhoneNumber()
  {
    return !TextUtils.isEmpty(get(Metadata.class).get(Metadata.MetadataType.FMD_PHONE_NUMBER));
  }

  public boolean hasAtm()
  {
    return has(RawData.class) && get(RawData.class).has("amenity-atm");
  }

  public boolean isTramStop()
  {
    return has(RawData.class) && get(RawData.class).has("railway-tram_stop");
  }

  public final boolean isMyPosition()
  {
    return mMapObjectType == MY_POSITION;
  }

  public final boolean isBookmark()
  {
    return mMapObjectType == BOOKMARK;
  }

  public final boolean isTrack()
  {
    return mMapObjectType == TRACK;
  }

  public final boolean isTrackRecording()
  {
    return mMapObjectType == TRACK_RECORDING;
  }

  @NonNull
  public String getDescription()
  {
    return "";
  }

  @Nullable
  public String getMetadata(@NonNull Metadata.MetadataType type)
  {
    if (has(Metadata.class))
      return get(Metadata.class).get(type);
    return null;
  }

  public boolean has(@NonNull Class<? extends MapObjectData> type)
  {
    return mData.containsKey(type);
  }

  @NonNull
  public <T extends MapObjectData> T get(@NonNull Class<T> type)
  {
    if (has(type))
      return Objects.requireNonNull(type.cast(mData.get(type)), "MapObjectData type " + type + " is not present");
    throw new IllegalArgumentException("MapObjectData type " + type + " is not present");
  }

  @VisibleForTesting
  @Keep
  void put(@NonNull MapObjectData value)
  {
    mData.put(value.getClass(), Objects.requireNonNull(value));
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    // A map object type must be written first, since it's used in readFromParcel method to distinguish
    // what type of object should be read from the parcel.
    dest.writeInt(mMapObjectType);
    dest.writeString(mTitle);
    dest.writeString(mSecondaryTitle);
    dest.writeString(mSubtitle);
    dest.writeString(mAddress);
    dest.writeDouble(mLat);
    dest.writeDouble(mLon);
    dest.writeInt(mData.size());
    for (final MapObjectData type : mData.values())
      dest.writeParcelable(type, flags);
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;

    return sameAs((MapObject) o);
  }

  @Override
  public int hashCode()
  {
    return Objects.hash(mMapObjectType, mTitle, mSubtitle, mLat, mLon);
  }

  public static final Creator<MapObject> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public MapObject createFromParcel(@NonNull Parcel source)
    {
      @MapObjectType
      int type = source.readInt();
      if (type == BOOKMARK)
        return new Bookmark(type, source);

      return new MapObject(type, source);
    }

    @Override
    @NonNull
    public MapObject[] newArray(int size)
    {
      return new MapObject[size];
    }
  };
}
