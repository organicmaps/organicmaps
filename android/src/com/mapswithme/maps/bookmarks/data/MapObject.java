package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.ads.Banner;
import com.mapswithme.maps.ads.LocalAdInfo;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.Popularity;
import com.mapswithme.maps.search.PopularityProvider;
import com.mapswithme.maps.search.PriceFilterView;
import com.mapswithme.maps.taxi.TaxiType;
import com.mapswithme.maps.ugc.UGC;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

// TODO(yunikkk): Refactor. Displayed information is different from edited information, and it's better to
// separate them. Simple getters from jni place_page::Info and osm::EditableFeature should be enough.
public class MapObject implements Parcelable, PopularityProvider
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

  @NonNull
  private final FeatureId mFeatureId;
  @MapObjectType
  private final int mMapObjectType;

  private String mTitle;
  @Nullable
  private String mSecondaryTitle;
  private String mSubtitle;
  private double mLat;
  private double mLon;
  private String mAddress;
  private Metadata mMetadata;
  private String mApiId;
  @Nullable
  private List<Banner> mBanners;
  @Nullable
  private List<TaxiType> mReachableByTaxiTypes;
  @Nullable
  private String mBookingSearchUrl;
  @Nullable
  private LocalAdInfo mLocalAdInfo;
  @Nullable
  private RoutePointInfo mRoutePointInfo;
  private final boolean mExtendedView;
  private final boolean mShouldShowUGC;
  private final boolean mCanBeRated;
  private final boolean mCanBeReviewed;
  @NonNull
  private final Popularity mPopularity;
  @NonNull
  private final String mDescription;
  @Nullable
  private ArrayList<UGC.Rating> mRatings;
  @Nullable
  private HotelsFilter.HotelType mHotelType;
  @PriceFilterView.PriceDef
  private final int mPriceRate;

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType, String title,
                   @Nullable String secondaryTitle, String subtitle, String address,
                   double lat, double lon, String apiId, @Nullable Banner[] banners,
                   @Nullable int[] types, @Nullable String bookingSearchUrl,
                   @Nullable LocalAdInfo localAdInfo, @Nullable RoutePointInfo routePointInfo,
                   boolean isExtendedView, boolean shouldShowUGC, boolean canBeRated,
                   boolean canBeReviewed, @Nullable UGC.Rating[] ratings,
                   @Nullable HotelsFilter.HotelType hotelType, @PriceFilterView.PriceDef int priceRate,
                   @NonNull Popularity popularity, @NonNull String description)
  {
    this(featureId, mapObjectType, title, secondaryTitle,
         subtitle, address, lat, lon, new Metadata(), apiId, banners,
         types, bookingSearchUrl, localAdInfo, routePointInfo, isExtendedView, shouldShowUGC,
         canBeRated, canBeReviewed, ratings, hotelType, priceRate, popularity, description);
  }

  public MapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType,
                   String title, @Nullable String secondaryTitle, String subtitle, String address,
                   double lat, double lon, Metadata metadata, String apiId,
                   @Nullable Banner[] banners, @Nullable int[] taxiTypes,
                   @Nullable String bookingSearchUrl, @Nullable LocalAdInfo localAdInfo,
                   @Nullable RoutePointInfo routePointInfo, boolean isExtendedView,
                   boolean shouldShowUGC, boolean canBeRated, boolean canBeReviewed,
                   @Nullable UGC.Rating[] ratings, @Nullable HotelsFilter.HotelType hotelType,
                   @PriceFilterView.PriceDef int priceRate, @NonNull Popularity popularity,
                   @NonNull String description)
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
    mBookingSearchUrl = bookingSearchUrl;
    mLocalAdInfo = localAdInfo;
    mRoutePointInfo = routePointInfo;
    mExtendedView = isExtendedView;
    mShouldShowUGC = shouldShowUGC;
    mCanBeRated = canBeRated;
    mCanBeReviewed = canBeReviewed;
    mPopularity = popularity;
    mDescription = description;
    if (banners != null)
      mBanners = new ArrayList<>(Arrays.asList(banners));
    if (taxiTypes != null)
    {
      mReachableByTaxiTypes = new ArrayList<>();
      for (int type : taxiTypes)
        mReachableByTaxiTypes.add(TaxiType.values()[type]);
    }
    if (ratings != null)
      mRatings = new ArrayList<>(Arrays.asList(ratings));
    mHotelType = hotelType;
    mPriceRate = priceRate;
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
         null, // mBanners
         null, // mReachableByTaxiTypes
         source.readString(), // BookingSearchUrl
         source.readParcelable(LocalAdInfo.class.getClassLoader()), // LocalAdInfo
         source.readParcelable(RoutePointInfo.class.getClassLoader()), // RoutePointInfo
         source.readInt() == 1, // mExtendedView
         source.readInt() == 1, // mShouldShowUGC
         source.readInt() == 1, // mCanBeRated;
         source.readInt() == 1, // mCanBeReviewed
         null, // mRatings
         source.readParcelable(HotelsFilter.HotelType.class.getClassLoader()), // mHotelType
         source.readInt(), // mPriceRate
         source.readParcelable(Popularity.class.getClassLoader()),
         source.readString()
        );

    mBanners = readBanners(source);
    mReachableByTaxiTypes = readTaxiTypes(source);
    mRatings = readRatings(source);
  }

  @NonNull
  public static MapObject createMapObject(@NonNull FeatureId featureId, @MapObjectType int mapObjectType,
                                          @NonNull String title, @NonNull String subtitle, double lat, double lon)
  {
    return new MapObject(featureId, mapObjectType, title,
                         "", subtitle, "", lat, lon, "", null,
                         null, "", null, null, false /* isExtendedView */,
                         false /* shouldShowUGC */, false /* canBeRated */, false /* canBeReviewed */,
                         null /* ratings */, null /* mHotelType */,
                         PriceFilterView.UNDEFINED, Popularity.defaultInstance(), "");
  }

  @Nullable
  private List<Banner> readBanners(@NonNull Parcel source)
  {
    List<Banner> banners = new ArrayList<>();
    source.readTypedList(banners, Banner.CREATOR);
    return banners.isEmpty() ? null : banners;
  }

  @Nullable
  private ArrayList<UGC.Rating> readRatings(@NonNull Parcel source)
  {
    ArrayList<UGC.Rating> ratings = new ArrayList<>();
    source.readTypedList(ratings, UGC.Rating.CREATOR);
    return ratings.isEmpty() ? null : ratings;
  }

  @NonNull
  private List<TaxiType> readTaxiTypes(@NonNull Parcel source)
  {
    List<TaxiType> types = new ArrayList<>();
    source.readList(types, TaxiType.class.getClassLoader());
    return types;
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

  public double getScale()
  {
    return 0;
  }

  public String getTitle()
  {
    return mTitle;
  }

  @Nullable
  public String getSecondaryTitle()
  {
    return mSecondaryTitle;
  }

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

  public String getAddress()
  {
    return mAddress;
  }

  @NonNull
  public String getDescription()
  {
    return mDescription;
  }

  @NonNull
  @Override
  public Popularity getPopularity()
  {
    return mPopularity;
  }

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
  public List<Banner> getBanners()
  {
    return mBanners;
  }

  @Nullable
  public ArrayList<UGC.Rating> getDefaultRatings()
  {
    return mRatings;
  }

  @Nullable
  public List<TaxiType> getReachableByTaxiTypes()
  {
    return mReachableByTaxiTypes;
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

  private void addMetadata(int type, String value)
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

  @Nullable
  public String getBookingSearchUrl()
  {
    return mBookingSearchUrl;
  }

  @Nullable
  public LocalAdInfo getLocalAdInfo()
  {
    return mLocalAdInfo;
  }

  @Nullable
  public RoutePointInfo getRoutePointInfo()
  {
    return mRoutePointInfo;
  }

  public boolean isExtendedView()
  {
    return mExtendedView;
  }

  public boolean shouldShowUGC()
  {
    return mShouldShowUGC;
  }

  public boolean canBeRated()
  {
    return mCanBeRated;
  }

  public boolean canBeReviewed()
  {
    return mCanBeReviewed;
  }

  @NonNull
  public FeatureId getFeatureId()
  {
    return mFeatureId;
  }

  @Nullable
  public HotelsFilter.HotelType getHotelType()
  {
    return mHotelType;
  }

  @PriceFilterView.PriceDef
  public int getPriceRate()
  {
    return mPriceRate;
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
    dest.writeString(mBookingSearchUrl);
    dest.writeParcelable(mLocalAdInfo, 0);
    dest.writeParcelable(mRoutePointInfo, 0);
    dest.writeInt(mExtendedView ? 1 : 0);
    dest.writeInt(mShouldShowUGC ? 1 : 0);
    dest.writeInt(mCanBeRated ? 1 : 0);
    dest.writeInt(mCanBeReviewed ? 1 : 0);
    dest.writeParcelable(mHotelType, 0);
    dest.writeInt(mPriceRate);
    dest.writeParcelable(mPopularity, 0);
    dest.writeString(mDescription);
    // All collections are deserialized AFTER non-collection and primitive type objects,
    // so collections must be always serialized at the end.
    dest.writeTypedList(mBanners);
    dest.writeList(mReachableByTaxiTypes);
    dest.writeTypedList(mRatings);
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
