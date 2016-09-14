package com.mapswithme.maps.widget.placepage;

import android.support.annotation.Nullable;
import android.support.annotation.DrawableRes;
import android.support.annotation.UiThread;
import android.text.TextUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;

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
    @DrawableRes
    private final int icon;
    private final String name;

    public FacilityType(@DrawableRes int icon, String name) {
      this.icon = icon;
      this.name = name;
    }

    @DrawableRes
    public int getIcon() {
      return icon;
    }

    public String getName() {
      return name;
    }
  }

  public static class Image {
    private final String url;

    public Image(String url) {
      this.url = url;
    }

    public String getUrl() {
      return url;
    }
  }

  interface OnPriceReceivedListener
  {
    void onPriceReceived(String id, String price, String currency);
  }

  interface OnDescriptionReceivedListener
  {
    void onDescriptionReceived(String id, String description);
  }

  interface OnFacilitiesReceivedListener
  {
    void onFacilitiesReceived(String id, List<FacilityType> facilities);
  }

  interface OnImagesReceivedListener
  {
    void onImagesReceived(String id, List<Image> images);
  }

  // Hotel ID -> Price
  private static final Map<String, Price> sPriceCache = new HashMap<>();
  // Hotel ID -> Description
  private static final Map<String, String> sDescriptionCache = new HashMap<>();
  // Hotel ID -> Facilities
  private static final Map<String, List<FacilityType>> sFacilitiesCache = new HashMap<>();
  // Hotel ID -> Images
  private static final Map<String, List<Image>> sImagesCache = new HashMap<>();
  private static WeakReference<OnPriceReceivedListener> sPriceListener;
  private static WeakReference<OnDescriptionReceivedListener> sDescriptionListener;
  private static WeakReference<OnFacilitiesReceivedListener> sFacilityListener;
  private static WeakReference<OnImagesReceivedListener> sImagesListener;

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

  public static void setDescriptionListener(OnDescriptionReceivedListener listener)
  {
    sDescriptionListener = new WeakReference<>(listener);
  }

  public static void setFacilitiesListener(OnFacilitiesReceivedListener listener)
  {
    sFacilityListener = new WeakReference<>(listener);
  }

  public static void setImagesListener(OnImagesReceivedListener listener)
  {
    sImagesListener = new WeakReference<>(listener);
  }

  @DrawableRes
  public static int mapFacilityId(int facilityId) {
//  TODO map facility id to drawable resource
    return R.drawable.ic_entrance;
  }

  static void requestPrice(String id, String currencyCode)
  {
    Price p = sPriceCache.get(id);
    if (p != null)
      onPriceReceived(id, p.price, p.currency);

    nativeRequestPrice(id, currencyCode);
  }

  static void requestDescription(String id, String locale)
  {
    String description = sDescriptionCache.get(id);
    if (description != null)
      onDescriptionReceived(id, description);

    nativeRequestDescription(id, locale);
  }

  static void requestFacilities(String id, String locale)
  {
    List<FacilityType> facilities = sFacilitiesCache.get(id);
    if (facilities != null) {
      OnFacilitiesReceivedListener listener = sFacilityListener.get();
      if (listener == null)
        sFacilityListener = null;
      else
        listener.onFacilitiesReceived(id, facilities);
    }

    nativeRequestFacilities(id, locale);
  }

  static void requestImages(String id, String locale)
  {
    List<Image> images = sImagesCache.get(id);
    if (images != null) {
      OnImagesReceivedListener listener = sImagesListener.get();
      if (listener == null)
        sImagesListener = null;
      else
        listener.onImagesReceived(id, images);
    }

    nativeRequestImages(id, locale);
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
  private static void onDescriptionReceived(String id, String description)
  {
    if (TextUtils.isEmpty(description))
      return;

    sDescriptionCache.put(id, description);

    OnDescriptionReceivedListener listener = sDescriptionListener.get();
    if (listener == null)
      sDescriptionListener = null;
    else
      listener.onDescriptionReceived(id, description);
  }

  @SuppressWarnings("unused")
  private static void onFacilitiesReceived(String id, int[] ids, String[] names)
  {
    if (ids.length == 0)
      return;

    List<FacilityType> result = new ArrayList<>();
    for (int i = 0; i < ids.length; i++) {
      result.add(new FacilityType(mapFacilityId(ids[i]), names[i]));
    }

    sFacilitiesCache.put(id, result);

    OnFacilitiesReceivedListener listener = sFacilityListener.get();
    if (listener == null)
      sFacilityListener = null;
    else
      listener.onFacilitiesReceived(id, result);
  }

  @SuppressWarnings("unused")
  private static void onImagesReceived(String id, String[] urls)
  {
    if (urls.length == 0)
      return;

    List<Image> result = new ArrayList<>();
    for (int i = 0; i < urls.length; i++) {
      result.add(new Image(urls[i]));
    }

    sImagesCache.put(id, result);

    OnImagesReceivedListener listener = sImagesListener.get();
    if (listener == null)
      sImagesListener = null;
    else
      listener.onImagesReceived(id, result);
  }

  @Nullable
  public static native SponsoredHotel nativeGetCurrent();
  private static native void nativeRequestPrice(String id, String currencyCode);
  private static native void nativeRequestDescription(String id, String locale);
  private static native void nativeRequestFacilities(String id, String locale);
  private static native void nativeRequestImages(String id, String locale);
}
