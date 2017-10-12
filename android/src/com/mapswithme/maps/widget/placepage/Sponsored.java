package com.mapswithme.maps.widget.placepage;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;
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
  public static final int TYPE_NONE = 0;
  public static final int TYPE_BOOKING = 1;
  public static final int TYPE_OPENTABLE = 2;
  public static final int TYPE_GEOCHAT = 3;
  public static final int TYPE_VIATOR = 4;
  public static final int TYPE_CIAN = 5;
  public static final int TYPE_THOR = 6;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_NONE, TYPE_BOOKING, TYPE_OPENTABLE, TYPE_GEOCHAT,
            TYPE_VIATOR, TYPE_CIAN, TYPE_THOR })
  public @interface SponsoredType {}

  private static class Price
  {
    @NonNull
    final String mPrice;
    @NonNull
    final String mCurrency;

    private Price(@NonNull String price, @NonNull String currency)
    {
      mPrice = price;
      mCurrency = currency;
    }
  }

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
     * @param id A hotel id
     * @param price A price
     * @param currency A price currency
     */
    @UiThread
    void onPriceReceived(@NonNull String id, @NonNull String price, @NonNull String currency);
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
  private static final Map<String, Price> sPriceCache = new HashMap<>();
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
  private final String mDescriptionUrl;
  @NonNull
  private final String mReviewUrl;
  @SponsoredType
  private final int mType;

  public Sponsored(@NonNull String rating, @UGC.Impress int impress, @NonNull String price, @NonNull String url,
                   @NonNull String descriptionUrl, @NonNull String reviewUrl,
                   @SponsoredType int type)
  {
    mRating = rating;
    mImpress = impress;
    mPrice = price;
    mUrl = url;
    mDescriptionUrl = descriptionUrl;
    mReviewUrl = reviewUrl;
    mType = type;
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
  String getDescriptionUrl()
  {
    return mDescriptionUrl;
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
   * and if cache exists - call {@link #onPriceReceived(String, String, String) onPriceReceived} immediately
   *  @param id A Hotel id
   * @param currencyCode A user currency
   * @param policy A network policy
   */
  static void requestPrice(@NonNull String id, @NonNull String currencyCode,
                           @NonNull NetworkPolicy policy)
  {
    Price p = sPriceCache.get(id);
    if (p != null)
      onPriceReceived(id, p.mPrice, p.mCurrency);

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
      case TYPE_GEOCHAT:
//        TODO: request geochat info
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

  private static void onPriceReceived(@NonNull String id, @NonNull String price,
                                      @NonNull String currency)
  {
    if (TextUtils.isEmpty(price))
      return;

    sPriceCache.put(id, new Price(price, currency));


    OnPriceReceivedListener listener = sPriceListener.get();
    if (listener != null)
      listener.onPriceReceived(id, price, currency);
  }

  private static void onHotelInfoReceived(@NonNull String id, @NonNull HotelInfo info)
  {
    sInfoCache.put(id, info);

    OnHotelInfoReceivedListener listener = sInfoListener.get();
    if (listener != null)
      listener.onHotelInfoReceived(id, info);
  }

  @Nullable
  public static native Sponsored nativeGetCurrent();

  private static native void nativeRequestPrice(@NonNull NetworkPolicy policy,
                                                @NonNull String id, @NonNull String currencyCode);

  private static native void nativeRequestHotelInfo(@NonNull NetworkPolicy policy,
                                                    @NonNull String id, @NonNull String locale);
}
