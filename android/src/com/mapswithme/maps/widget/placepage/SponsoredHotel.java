package com.mapswithme.maps.widget.placepage;

import com.google.android.gms.maps.model.LatLng;

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

  public static class NearbyObject {
    private final String category;
    private final String title;
    private final String distance;
    private final LatLng location;

    public NearbyObject(String category, String title, String distance, LatLng location) {
      this.category = category;
      this.title = title;
      this.distance = distance;
      this.location = location;
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

    public LatLng getLocation() {
      return location;
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
    void onImagesReceived(String id, ArrayList<Image> images);
  }

  interface OnNearbyReceivedListener
  {
    void onNearbyReceived(String id, ArrayList<NearbyObject> images);
  }

  // Hotel ID -> Price
  private static final Map<String, Price> sPriceCache = new HashMap<>();
  // Hotel ID -> Description
  private static final Map<String, String> sDescriptionCache = new HashMap<>();
  // Hotel ID -> Facilities
  private static final Map<String, List<FacilityType>> sFacilitiesCache = new HashMap<>();
  // Hotel ID -> Images
  private static final Map<String, ArrayList<Image>> sImagesCache = new HashMap<>();
  // Hotel ID -> Nearby
  private static final Map<String, ArrayList<NearbyObject>> sNearbyCache = new HashMap<>();
  private static WeakReference<OnPriceReceivedListener> sPriceListener;
  private static WeakReference<OnDescriptionReceivedListener> sDescriptionListener;
  private static WeakReference<OnFacilitiesReceivedListener> sFacilityListener;
  private static WeakReference<OnImagesReceivedListener> sImagesListener;
  private static WeakReference<OnNearbyReceivedListener> sNearbyListener;

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

  public static void setNearbyListener(OnNearbyReceivedListener listener)
  {
    sNearbyListener = new WeakReference<>(listener);
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
    ArrayList<Image> images = sImagesCache.get(id);
    if (images != null) {
      OnImagesReceivedListener listener = sImagesListener.get();
      if (listener == null)
        sImagesListener = null;
      else
        listener.onImagesReceived(id, images);
    }

    nativeRequestImages(id, locale);
  }

  static void requestNearby(String id, LatLng position)
  {
    ArrayList<NearbyObject> objects = sNearbyCache.get(id);
    if (objects != null) {
      OnNearbyReceivedListener listener = sNearbyListener.get();
      if (listener == null)
        sNearbyListener = null;
      else
        listener.onNearbyReceived(id, objects);
    }

    nativeRequestNearby(id, position.latitude, position.longitude);
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

    ArrayList<Image> result = new ArrayList<>();
    for (String url: urls) {
      Image image = new Image(url);
      image.setDescription("Staff, rooftop view, location, free bike…");
      image.setDate(System.currentTimeMillis());
      image.setSource("via Booking");
      image.setUserAvatar("http://www.interdating-ukrainian-women.com/wp-content/uploads/2013/06/avatar.jpg");
      image.setUserName("Polina");
      result.add(image);
    }

    sImagesCache.put(id, result);

    OnImagesReceivedListener listener = sImagesListener.get();
    if (listener == null)
      sImagesListener = null;
    else
      listener.onImagesReceived(id, result);
  }

  @SuppressWarnings("unused")
  private static void onNearbyReceived(String id)
  {
    ArrayList<NearbyObject> result = new ArrayList<>();
    result.add(new NearbyObject("transport", "Bowery", "800 ft", new LatLng(0, 0)));
    result.add(new NearbyObject("food", "Egg Shop", "300 ft", new LatLng(0, 0)));
    result.add(new NearbyObject("shop", "Fay Yee Inc", "200 ft", new LatLng(0, 0)));

    sNearbyCache.put(id, result);

    OnNearbyReceivedListener listener = sNearbyListener.get();
    if (listener == null)
      sNearbyListener = null;
    else
      listener.onNearbyReceived(id, result);
  }

  @Nullable
  public static native SponsoredHotel nativeGetCurrent();
  private static native void nativeRequestPrice(String id, String currencyCode);
  private static native void nativeRequestDescription(String id, String locale);
  private static native void nativeRequestFacilities(String id, String locale);
  private static native void nativeRequestImages(String id, String locale);
  private static native void nativeRequestNearby(String id, double lat, double lon);
}
