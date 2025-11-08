package app.organicmaps.sdk.bookmarks.data;

import android.net.Uri;
import android.os.Parcel;
import android.text.TextUtils;
import androidx.annotation.IntDef;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.os.ParcelCompat;
import app.organicmaps.sdk.routing.RoutePointInfo;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Arrays;
import java.util.List;
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

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({OPENING_MODE_PREVIEW, OPENING_MODE_PREVIEW_PLUS, OPENING_MODE_DETAILS, OPENING_MODE_FULL})
  public @interface OpeningMode
  {}

  public static final int OPENING_MODE_PREVIEW = 0;
  public static final int OPENING_MODE_PREVIEW_PLUS = 1;
  public static final int OPENING_MODE_DETAILS = 2;
  public static final int OPENING_MODE_FULL = 3;

  private static final String kHttp = "http://";
  private static final String kHttps = "https://";

  @MapObjectType
  private final int mMapObjectType;

  private String mTitle;
  @Nullable
  private final String mSecondaryTitle;
  private final String mSubtitle;
  private final String mAddress;
  private double mLat;
  private double mLon;
  @NonNull
  private final Metadata mMetadata;
  private final String mApiId;
  private final RoutePointInfo mRoutePointInfo;
  @OpeningMode
  private final int mOpeningMode;
  @NonNull
  private String mWikiArticle;
  @NonNull
  private final String mOsmDescription;
  @NonNull
  private final RoadWarningMarkType mRoadWarningMarkType;
  @Nullable
  private List<String> mRawTypes;

  public MapObject(@MapObjectType int mapObjectType, String title, @Nullable String secondaryTitle, String subtitle,
                   String address, double lat, double lon, String apiId, @Nullable RoutePointInfo routePointInfo,
                   @OpeningMode int openingMode, @NonNull String wikiArticle, @NonNull String osmDescription,
                   int roadWarningType, @Nullable String[] rawTypes)
  {
    this(mapObjectType, title, secondaryTitle, subtitle, address, lat, lon, new Metadata(), apiId, routePointInfo,
         openingMode, wikiArticle, osmDescription, roadWarningType, rawTypes);
  }

  public MapObject(@MapObjectType int mapObjectType, String title, @Nullable String secondaryTitle, String subtitle,
                   String address, double lat, double lon, @Nullable Metadata metadata, String apiId,
                   @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode, @NonNull String wikiArticle,
                   @NonNull String osmDescription, int roadWarningType, @Nullable String[] rawTypes)
  {
    mMapObjectType = mapObjectType;
    mTitle = title;
    mSecondaryTitle = secondaryTitle;
    mSubtitle = subtitle;
    mAddress = address;
    mLat = lat;
    mLon = lon;
    mMetadata = metadata != null ? metadata : new Metadata();
    mApiId = apiId;
    mRoutePointInfo = routePointInfo;
    mOpeningMode = openingMode;
    mWikiArticle = wikiArticle;
    mOsmDescription = osmDescription;
    mRoadWarningMarkType = RoadWarningMarkType.values()[roadWarningType];
    if (rawTypes != null)
      mRawTypes = Arrays.asList(rawTypes);
  }

  // Also called from Bookmark constructor.
  protected MapObject(@MapObjectType int type, @NonNull Parcel source)
  {
    // Type has already been read in readFromParcel method.
    mMapObjectType = type;
    // Reading order must be the same as writing order in writeToParcel.
    mTitle = source.readString();
    mSecondaryTitle = source.readString();
    mSubtitle = source.readString();
    mAddress = source.readString();
    mLat = source.readDouble();
    mLon = source.readDouble();
    mMetadata = ParcelCompat.readParcelable(source, Metadata.class.getClassLoader(), Metadata.class);
    mApiId = source.readString();
    mRoutePointInfo = ParcelCompat.readParcelable(source, RoutePointInfo.class.getClassLoader(), RoutePointInfo.class);
    mOpeningMode = source.readInt();
    mWikiArticle = Objects.requireNonNull(source.readString());
    mOsmDescription = source.readString();
    mRoadWarningMarkType = RoadWarningMarkType.values()[source.readInt()];
    source.readStringList(mRawTypes);
  }

  @NonNull
  public static MapObject createMapObject(@MapObjectType int mapObjectType, @NonNull String title,
                                          @NonNull String subtitle, double lat, double lon)
  {
    return new MapObject(mapObjectType, title, "", subtitle, "", lat, lon, null, "", null, OPENING_MODE_PREVIEW, "", "",
                         RoadWarningMarkType.UNKNOWN.ordinal(), new String[0]);
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

    return mMapObjectType == other.mMapObjectType && mTitle.equals(other.mTitle) && mSubtitle.equals(other.mTitle)
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
  public String getWikiArticle()
  {
    return mWikiArticle;
  }

  public void setWikiArticle(@NonNull String wikiArticle)
  {
    mWikiArticle = wikiArticle;
  }

  @NonNull
  public String getOsmDescription()
  {
    return mOsmDescription;
  }

  @NonNull
  public RoadWarningMarkType getRoadWarningMarkType()
  {
    return mRoadWarningMarkType;
  }

  @NonNull
  public String getMetadata(Metadata.MetadataType type)
  {
    final String res = mMetadata.getMetadata(type);
    return res == null ? "" : res;
  }

  @NonNull
  public String getWebsiteUrl(boolean strip, @NonNull Metadata.MetadataType type)
  {
    final String website = Uri.decode(getMetadata(type));
    final int len = website.length();
    if (strip && len > 1)
    {
      final int start = website.startsWith(kHttps) ? kHttps.length() : (website.startsWith(kHttp) ? kHttp.length() : 0);
      final int end = website.endsWith("/") ? len - 1 : len;
      return website.substring(start, end);
    }
    return website;
  }

  public String getApiId()
  {
    return mApiId;
  }

  public void setLat(double lat)
  {
    mLat = lat;
  }

  public void setLon(double lon)
  {
    mLon = lon;
  }

  // Called from JNI.
  @Keep
  @SuppressWarnings("unused")
  public void addMetadata(int type, String value)
  {
    mMetadata.addMetadata(type, value);
  }

  public boolean hasPhoneNumber()
  {
    return !TextUtils.isEmpty(getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
  }

  public boolean hasAtm()
  {
    if (mRawTypes == null)
      return false;
    return mRawTypes.contains("amenity-atm");
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

  @Nullable
  public RoutePointInfo getRoutePointInfo()
  {
    return mRoutePointInfo;
  }

  @OpeningMode
  public int getOpeningMode()
  {
    return mOpeningMode;
  }

  private static MapObject readFromParcel(Parcel source)
  {
    @MapObjectType
    int type = source.readInt();
    if (type == BOOKMARK)
      return new Bookmark(type, source);

    return new MapObject(type, source);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
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
    dest.writeParcelable(mMetadata, 0);
    dest.writeString(mApiId);
    dest.writeParcelable(mRoutePointInfo, 0);
    dest.writeInt(mOpeningMode);
    dest.writeString(mWikiArticle);
    dest.writeString(mOsmDescription);
    dest.writeInt(getRoadWarningMarkType().ordinal());
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeStringList(mRawTypes);
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
