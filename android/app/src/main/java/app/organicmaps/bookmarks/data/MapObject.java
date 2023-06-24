package app.organicmaps.bookmarks.data;

import android.os.Parcel;
import android.text.TextUtils;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.routing.RoutePointInfo;
import app.organicmaps.search.Popularity;
import app.organicmaps.search.PopularityProvider;
import app.organicmaps.widget.placepage.PlacePageData;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// TODO(yunikkk): Refactor. Displayed information is different from edited information, and it's better to
// separate them. Simple getters from jni place_page::Info and osm::EditableFeature should be enough.
public class MapObject implements PopularityProvider, PlacePageData
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ POI, API_POINT, BOOKMARK, MY_POSITION, SEARCH })
  public @interface MapObjectType
  {
  }

  public static final int POI = 0;
  public static final int API_POINT = 1;
  public static final int BOOKMARK = 2;
  public static final int MY_POSITION = 3;
  public static final int SEARCH = 4;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ OPENING_MODE_PREVIEW, OPENING_MODE_PREVIEW_PLUS, OPENING_MODE_DETAILS, OPENING_MODE_FULL })
  public @interface OpeningMode {}

  public static final int OPENING_MODE_PREVIEW = 0;
  public static final int OPENING_MODE_PREVIEW_PLUS = 1;
  public static final int OPENING_MODE_DETAILS = 2;
  public static final int OPENING_MODE_FULL = 3;

  @NonNull
  private final FeatureId mFeatureId;
  @MapObjectType
  private final int mMapObjectType;

  private String mTitle;
  @Nullable
  private final String mSecondaryTitle;
  private String mSubtitle;
  private double mLat;
  private double mLon;
  private final String mAddress;
  private final Metadata mMetadata;
  private final String mApiId;
  private final RoutePointInfo mRoutePointInfo;
  @OpeningMode
  private final int mOpeningMode;
  @NonNull
  private final Popularity mPopularity;
  @NonNull
  private final RoadWarningMarkType mRoadWarningMarkType;
  @NonNull
  private String mDescription;
  @Nullable
  private List<String> mRawTypes;

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
                   @Nullable String secondaryTitle, String subtitle, String address,
                   double lat, double lon, String apiId, @Nullable RoutePointInfo routePointInfo,
                   @OpeningMode int openingMode, @NonNull Popularity popularity, @NonNull String description,
                   int roadWarningType, @Nullable String[] rawTypes)
  {
    this(featureId, mapObjectType, title, secondaryTitle,
        subtitle, address, lat, lon, new Metadata(), apiId,
        routePointInfo, openingMode, popularity, description,
        roadWarningType, rawTypes);
  }

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType,
                   String title, @Nullable String secondaryTitle, String subtitle, String address,
                   double lat, double lon, Metadata metadata, String apiId,
                   @Nullable RoutePointInfo routePointInfo, @OpeningMode int openingMode, @NonNull Popularity popularity,
                   @NonNull String description, int roadWarningType, @Nullable String[] rawTypes)
  {
    mFeatureId = featureId;
    mMapObjectType = mapObjectType;
    mTitle = title;
    mSecondaryTitle = secondaryTitle;
    mSubtitle = subtitle;
    mAddress = address;
    mLat = lat;
    mLon = lon;
    mMetadata = metadata;
    mApiId = apiId;
    mRoutePointInfo = routePointInfo;
    mOpeningMode = openingMode;
    mPopularity = popularity;
    mDescription = description;
    mRoadWarningMarkType = RoadWarningMarkType.values()[roadWarningType];
    if (rawTypes != null)
      mRawTypes = new ArrayList<>(Arrays.asList(rawTypes));
  }

  protected MapObject(@MapObjectType int type, Parcel source)
  {
    //noinspection ResourceType
    this(source.readParcelable(FeatureId.class.getClassLoader()), // FeatureId
         type, // MapObjectType
         source.readString(), // Title
         source.readString(), // SecondaryTitle
         source.readString(), // Subtitle
         source.readString(), // Address
         source.readDouble(), // Lat
         source.readDouble(), // Lon
         source.readParcelable(Metadata.class.getClassLoader()),
         source.readString(), // ApiId;
         source.readParcelable(RoutePointInfo.class.getClassLoader()), // RoutePointInfo
         source.readInt(), // mOpeningMode
         source.readParcelable(Popularity.class.getClassLoader()),
         source.readString(),
         source.readInt(),
         null // mRawTypes
        );

    mRawTypes = readRawTypes(source);
  }

  @NonNull
  public static MapObject createMapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType,
                                          @NonNull String title, @NonNull String subtitle, double lat, double lon)
  {
    return new MapObject(featureId, mapObjectType, title,
                         "", subtitle, "", lat, lon, null,
                         "", null, OPENING_MODE_PREVIEW,
                         Popularity.defaultInstance(), "",
                         RoadWarningMarkType.UNKNOWN.ordinal(), new String[0]);
  }

  @NonNull
  private static List<String> readRawTypes(@NonNull Parcel source)
  {
    List<String> types = new ArrayList<>();
    source.readStringList(types);
    return types;
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

    //noinspection SimplifiableIfStatement
    if (getClass() != other.getClass())
      return false;

    if (mFeatureId != FeatureId.EMPTY && other.getFeatureId() != FeatureId.EMPTY)
      return mFeatureId.equals(other.getFeatureId());

    return Double.doubleToLongBits(mLon) == Double.doubleToLongBits(other.mLon) &&
           Double.doubleToLongBits(mLat) == Double.doubleToLongBits(other.mLat);
  }

  public static boolean same(@Nullable MapObject one, @Nullable MapObject another)
  {
    //noinspection SimplifiableIfStatement
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
  public String getDescription()
  {
    return mDescription;
  }

  public void setDescription(@NonNull String description)
  {
    mDescription = description;
  }

  @NonNull
  @Override
  public Popularity getPopularity()
  {
    return mPopularity;
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

  public boolean hasMetadata()
  {
    return mMetadata != null && !mMetadata.isEmpty();
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

  @NonNull
  public  String[] getRawTypes()
  {
    if (mRawTypes == null)
      return new String[0];

    String[] types = new String[mRawTypes.size()];
    mRawTypes.toArray(types);
    return types;
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

  private void addMetadata(int type, String value)
  {
    mMetadata.addMetadata(type, value);
  }

  public boolean hasPhoneNumber()
  {
    return !TextUtils.isEmpty(getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
  }

  public static boolean isOfType(@MapObjectType int type, MapObject object)
  {
    return object != null && object.getMapObjectType() == type;
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

  @NonNull
  public FeatureId getFeatureId()
  {
    return mFeatureId;
  }

  private static MapObject readFromParcel(Parcel source)
  {
    @MapObjectType int type = source.readInt();
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
    // A map object type must be written first, since it's used in readParcel method to distinguish
    // what type of object should be read from the parcel.
    dest.writeInt(mMapObjectType);
    dest.writeParcelable(mFeatureId, 0);
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
    dest.writeParcelable(mPopularity, 0);
    dest.writeString(mDescription);
    dest.writeInt(getRoadWarningMarkType().ordinal());
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeStringList(mRawTypes);
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    MapObject mapObject = (MapObject) o;
    return mFeatureId.equals(mapObject.mFeatureId);
  }

  @Override
  public int hashCode()
  {
    return mFeatureId.hashCode();
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
