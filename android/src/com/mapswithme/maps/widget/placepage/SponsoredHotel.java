package com.mapswithme.maps.widget.placepage;

import android.support.annotation.Nullable;
import android.support.annotation.UiThread;
import android.text.TextUtils;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

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

  interface OnPriceReceivedListener
  {
    void onPriceReceived(String id, String price, String currency);
  }

  interface OnDescriptionReceivedListener
  {
    void onDescriptionReceived(String id, String description);
  }

  // Hotel ID -> Price
  private static final Map<String, Price> sPriceCache = new HashMap<>();
  // Hotel ID -> Description
  private static final Map<String, String> sDescriptionCache = new HashMap<>();
  private static WeakReference<OnPriceReceivedListener> sPriceListener;
  private static WeakReference<OnDescriptionReceivedListener> sDescriptionListener;

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

  @Nullable
  public static native SponsoredHotel nativeGetCurrent();
  private static native void nativeRequestPrice(String id, String currencyCode);
  private static native void nativeRequestDescription(String id, String locale);
}
