package com.mapswithme.maps.widget.placepage;

import android.support.annotation.Nullable;
import android.support.annotation.UiThread;
import android.text.TextUtils;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.gallery.Image;

@UiThread
public final class SponsoredHotel
{
  private static class Price
  {
    final String price;
    final String currency;

    private Price(String price, String currency)
    {
      this.price = price;
      this.currency = currency;
    }
  }

  public static class FacilityType {
    private final String key;
    private final String name;

    public FacilityType(String key, String name) {
      this.key = key;
      this.name = name;
    }

    public String getKey() {
      return key;
    }

    public String getName() {
      return name;
    }
  }

  public static class NearbyObject {
    private final String category;
    private final String title;
    private final String distance;
    private final double latitude;
    private final double longitude;

    public NearbyObject(String category, String title, String distance, double lat, double lon) {
      this.category = category;
      this.title = title;
      this.distance = distance;
      this.latitude = lat;
      this.longitude = lon;
    }

    public String getCategory() {
      return category;
    }

    public String getTitle() {
      return title;
    }

    public String getDistance() {
      return distance;
    }

    public double getLatitude() {
      return latitude;
    }

    public double getLongitude() {
      return longitude;
    }
  }

  public static class Review {
    private final String mReviewPositive;
    private final String mReviewNegative;
    private final String mAuthor;
    private final String mAuthorAvatar;
    private final float mRating;
    private final long mDate;

    public Review(String reviewPositive, String reviewNegative, String author, String authorAvatar,
            float rating, long date) {
      mReviewPositive = reviewPositive;
      mReviewNegative = reviewNegative;
      mAuthor = author;
      mAuthorAvatar = authorAvatar;
      mRating = rating;
      mDate = date;
    }

    public String getReviewPositive() {
      return mReviewPositive;
    }

    public String getReviewNegative() {
      return mReviewNegative;
    }

    public String getAuthor() {
      return mAuthor;
    }

    public String getAuthorAvatar() {
      return mAuthorAvatar;
    }

    public float getRating() {
      return mRating;
    }

    public long getDate() {
      return mDate;
    }
  }

  public static class HotelInfo {
    final String description;
    final Image[] photos;
    final FacilityType[] facilities;
    final Review[] reviews;
    final NearbyObject[] nearby;

    public HotelInfo(String description, Image[] photos,
            FacilityType[] facilities, Review[] reviews,
            NearbyObject[] nearby) {
      this.description = description;
      this.photos = photos;
      this.facilities = facilities;
      this.reviews = reviews;
      this.nearby = nearby;
    }
  }

  interface OnPriceReceivedListener
  {
    void onPriceReceived(String id, String price, String currency);
  }

  interface OnInfoReceivedListener
  {
    void onInfoReceived(String id, HotelInfo info);
  }

  // Hotel ID -> Price
  private static final Map<String, Price> sPriceCache = new HashMap<>();
  // Hotel ID -> Description
  private static final Map<String, HotelInfo> sInfoCache = new HashMap<>();
  private static WeakReference<OnPriceReceivedListener> sPriceListener;
  private static WeakReference<OnInfoReceivedListener> sInfoListener;

  private String mId;

  final String rating;
  final String price;
  final String urlBook;
  final String urlDescription;

  public SponsoredHotel(String rating, String price, String urlBook, String urlDescription)
  {
    this.rating = rating;
    this.price = price;
    this.urlBook = urlBook;
    this.urlDescription = urlDescription;
  }

  void updateId(MapObject point)
  {
    mId = point.getMetadata(Metadata.MetadataType.FMD_SPONSORED_ID);
  }

  public String getId()
  {
    return mId;
  }

  public String getRating()
  {
    return rating;
  }

  public String getPrice()
  {
    return price;
  }

  public String getUrlBook()
  {
    return urlBook;
  }

  public String getUrlDescription()
  {
    return urlDescription;
  }

  public static void setPriceListener(OnPriceReceivedListener listener)
  {
    sPriceListener = new WeakReference<>(listener);
  }

  public static void setInfoListener(OnInfoReceivedListener listener)
  {
    sInfoListener = new WeakReference<>(listener);
  }

  static void requestPrice(String id, String currencyCode)
  {
    Price p = sPriceCache.get(id);
    if (p != null)
      onPriceReceived(id, p.price, p.currency);

    nativeRequestPrice(id, currencyCode);
  }

  static void requestInfo(String id, String locale)
  {
    HotelInfo info = sInfoCache.get(id);
    if (info != null)
      onInfoReceived(id, info);

    nativeRequestInfo(id, locale);
  }

  @SuppressWarnings("unused")
  private static void onPriceReceived(String id, String price, String currency)
  {
    if (TextUtils.isEmpty(price))
      return;

    sPriceCache.put(id, new Price(price, currency));

    OnPriceReceivedListener listener = sPriceListener.get();
    if (listener == null)
      sPriceListener = null;
    else
      listener.onPriceReceived(id, price, currency);
  }

  @SuppressWarnings("unused")
  private static void onInfoReceived(String id, HotelInfo info)
  {
    if (info == null)
      return;

    sInfoCache.put(id, info);

    OnInfoReceivedListener listener = sInfoListener.get();
    if (listener == null)
      sInfoListener = null;
    else
      listener.onInfoReceived(id, info);
  }

  @Nullable
  public static native SponsoredHotel nativeGetCurrent();
  private static native void nativeRequestPrice(String id, String currencyCode);
  private static native void nativeRequestInfo(String id, String locale);
}
