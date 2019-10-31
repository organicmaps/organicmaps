package com.mapswithme.maps.widget.placepage;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import android.text.TextUtils;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.gallery.Image;
import com.mapswithme.maps.review.Review;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.util.NetworkPolicy;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

@UiThread
public final class Sponsored
{
  // Order is important, must match place_page_info.hpp/SponsoredType.
  public static final int TYPE_NONE = 0;
  public static final int TYPE_BOOKING = 1;
  public static final int TYPE_OPENTABLE = 2;
  public static final int TYPE_PARTNER = 3;
  public static final int TYPE_HOLIDAY = 4;
  public static final int TYPE_PROMO_CATALOG_CITY = 5;
  public static final int TYPE_PROMO_CATALOG_SIGHTSEEINGS = 6;
  public static final int TYPE_PROMO_CATALOG_OUTDOOR = 7;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_NONE, TYPE_BOOKING, TYPE_OPENTABLE, TYPE_PARTNER, TYPE_HOLIDAY,
            TYPE_PROMO_CATALOG_CITY, TYPE_PROMO_CATALOG_SIGHTSEEINGS, TYPE_PROMO_CATALOG_OUTDOOR })
  public @interface SponsoredType {}

  static class FacilityType
  {
    @NonNull
    private final String mKey;
    @NonNull
    private final String mName;

    public FacilityType(@NonNull String key, @NonNull String name)
    {
      mKey = key;
      mName = name;
    }

    @NonNull
    public String getKey()
    {
      return mKey;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }
  }

  static class NearbyObject
  {
    @NonNull
    private final String mCategory;
    @NonNull
    private final String mTitle;
    @NonNull
    private final String mDistance;
    private final double mLatitude;
    private final double mLongitude;

    public NearbyObject(@NonNull String category, @NonNull String title,
                        @NonNull String distance, double lat, double lon)
    {
      mCategory = category;
      mTitle = title;
      mDistance = distance;
      mLatitude = lat;
      mLongitude = lon;
    }

    @NonNull
    public String getCategory()
    {
      return mCategory;
    }

    @NonNull
    public String getTitle()
    {
      return mTitle;
    }

    @NonNull
    public String getDistance()
    {
      return mDistance;
    }

    public double getLatitude()
    {
      return mLatitude;
    }

    public double getLongitude()
    {
      return mLongitude;
    }
  }

  static class HotelInfo
  {
    @Nullable
    final String mDescription;
    @Nullable
    final Image[] mPhotos;
    @Nullable
    final FacilityType[] mFacilities;
    @Nullable
    final Review[] mReviews;
    @Nullable
    final NearbyObject[] mNearby;
    final long mReviewsAmount;


    public HotelInfo(@Nullable String description, @Nullable Image[] photos,
                     @Nullable FacilityType[] facilities, @Nullable Review[] reviews,
                     @Nullable NearbyObject[] nearby, long reviewsAmount)
    {
      mDescription = description;
      mPhotos = photos;
      mFacilities = facilities;
      mReviews = reviews;
      mNearby = nearby;
      mReviewsAmount = reviewsAmount;
    }
  }

  interface OnPriceReceivedListener
  {
    /**
     * This method is called from the native core on the UI thread
     * when the Hotel price will be obtained
     *
     * @param priceInfo
     */
    @UiThread
    void onPriceReceived(@NonNull HotelPriceInfo priceInfo);
  }

  interface OnHotelInfoReceivedListener
  {
    /**
     * This method is called from the native core on the UI thread
     * when the Hotel information will be obtained
     *
     * @param id A hotel id
     * @param info A hotel info
     */
    @UiThread
    void onHotelInfoReceived(@NonNull String id, @NonNull HotelInfo info);
  }

  // Hotel ID -> Price
  @NonNull
  private static final Map<String, HotelPriceInfo> sPriceCache = new HashMap<>();
  // Hotel ID -> Description
  @NonNull
  private static final Map<String, HotelInfo> sInfoCache = new HashMap<>();
  @NonNull
  private static WeakReference<OnPriceReceivedListener> sPriceListener = new WeakReference<>(null);
  @NonNull
  private static WeakReference<OnHotelInfoReceivedListener> sInfoListener = new WeakReference<>(null);

  @Nullable
  private String mId;

  @NonNull
  private final String mRating;
  @UGC.Impress
  private final int mImpress;
  @NonNull
  private final String mPrice;
  @NonNull
  private final String mUrl;
  @NonNull
  private final String mDeepLink;
  @NonNull
  private final String mDescriptionUrl;
  @NonNull
  private final String mMoreUrl;
  @NonNull
  private final String mReviewUrl;
  @SponsoredType
  private final int mType;
  private final int mPartnerIndex;
  @NonNull
  private final String mPartnerName;

  private Sponsored(@NonNull String rating, @UGC.Impress int impress, @NonNull String price,
                    @NonNull String url, @NonNull String deepLink, @NonNull String descriptionUrl,
                    @NonNull String moreUrl, @NonNull String reviewUrl, @SponsoredType int type,
                    int partnerIndex, @NonNull String partnerName)
  {
    mRating = rating;
    mImpress = impress;
    mPrice = price;
    mUrl = url;
    mDeepLink = deepLink;
    mDescriptionUrl = descriptionUrl;
    mMoreUrl = moreUrl;
    mReviewUrl = reviewUrl;
    mType = type;
    mPartnerIndex = partnerIndex;
    mPartnerName = partnerName;
  }

  void updateId(MapObject point)
  {
    mId = point.getMetadata(Metadata.MetadataType.FMD_SPONSORED_ID);
  }

  @Nullable
  public String getId()
  {
    return mId;
  }

  @NonNull
  public String getRating()
  {
    return mRating;
  }

  @UGC.Impress
  int getImpress()
  {
    return mImpress;
  }

  @NonNull
  String getPrice()
  {
    return mPrice;
  }

  @NonNull
  public String getUrl()
  {
    return mUrl;
  }

  @NonNull
  public String getDeepLink()
  {
    return mDeepLink;
  }

  @NonNull
  String getDescriptionUrl()
  {
    return mDescriptionUrl;
  }

  @NonNull
  String getMoreUrl()
  {
    return mMoreUrl;
  }

  @NonNull
  String getReviewUrl()
  {
    return mReviewUrl;
  }

  @SponsoredType
  public int getType()
  {
    return mType;
  }

  public int getPartnerIndex()
  {
    return mPartnerIndex;
  }

  @NonNull
  public String getPartnerName()
  {
    return mPartnerName;
  }

  static void setPriceListener(@NonNull OnPriceReceivedListener listener)
  {
    sPriceListener = new WeakReference<>(listener);
  }

  static void setInfoListener(@NonNull OnHotelInfoReceivedListener listener)
  {
    sInfoListener = new WeakReference<>(listener);
  }

  /**
   * Make request to obtain hotel price information.
   * This method also checks cache for requested hotel id
   * and if cache exists - call {@link #onPriceReceived(HotelPriceInfo) onPriceReceived} immediately
   *  @param id A Hotel id
   * @param currencyCode A user currency
   * @param policy A network policy
   */
  static void requestPrice(@NonNull String id, @NonNull String currencyCode,
                           @NonNull NetworkPolicy policy)
  {
    HotelPriceInfo p = sPriceCache.get(id);
    if (p != null)
      onPriceReceived(p);

    nativeRequestPrice(policy, id, currencyCode);
  }


  static void requestInfo(@NonNull Sponsored sponsored,
                          @NonNull String locale, @NonNull NetworkPolicy policy)
  {
    String id = sponsored.getId();
    if (id == null)
      return;

    switch (sponsored.getType())
    {
      case TYPE_BOOKING:
        requestHotelInfo(id, locale, policy);
        break;
      case TYPE_OPENTABLE:
//        TODO: request opentable info
        break;
      case TYPE_NONE:
        break;
    }
  }

  /**
   * Make request to obtain hotel information.
   * This method also checks cache for requested hotel id
   * and if cache exists - call {@link #onHotelInfoReceived(String, HotelInfo) onHotelInfoReceived} immediately
   *  @param id A Hotel id
   * @param locale A user locale
   * @param policy A network policy
   */
  private static void requestHotelInfo(@NonNull String id, @NonNull String locale,
                                       @NonNull NetworkPolicy policy)
  {
    HotelInfo info = sInfoCache.get(id);
    if (info != null)
      onHotelInfoReceived(id, info);

    nativeRequestHotelInfo(policy, id, locale);
  }

  private static void onPriceReceived(@NonNull HotelPriceInfo priceInfo)
  {
    if (TextUtils.isEmpty(priceInfo.getPrice()))
      return;

    sPriceCache.put(priceInfo.getId(), priceInfo);


    OnPriceReceivedListener listener = sPriceListener.get();
    if (listener != null)
      listener.onPriceReceived(priceInfo);
  }

  private static void onHotelInfoReceived(@NonNull String id, @NonNull HotelInfo info)
  {
    sInfoCache.put(id, info);

    OnHotelInfoReceivedListener listener = sInfoListener.get();
    if (listener != null)
      listener.onHotelInfoReceived(id, info);
  }

  @NonNull
  static String getPackageName(@SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_BOOKING:
        return "com.booking";
      default:
        throw new AssertionError("Unsupported sponsored type: " + type);
    }
  }

  @Nullable
  public static native Sponsored nativeGetCurrent();

  private static native void nativeRequestPrice(@NonNull NetworkPolicy policy,
                                                @NonNull String id, @NonNull String currencyCode);

  private static native void nativeRequestHotelInfo(@NonNull NetworkPolicy policy,
                                                    @NonNull String id, @NonNull String locale);
}
