package com.mapswithme.maps.widget.placepage;

import android.support.annotation.UiThread;
import android.text.TextUtils;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;

@UiThread
final class SponsoredHotel
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

  // Hotel ID -> Price
  private static final Map<String, Price> sPriceCache = new HashMap<>();
  private static WeakReference<OnPriceReceivedListener> sListener;

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

  public static void setListener(OnPriceReceivedListener listener)
  {
    sListener = new WeakReference<>(listener);
  }

  static void requestPrice(String id, String currencyCode)
  {
    Price p = sPriceCache.get(id);
    if (p != null)
      onPriceReceived(id, p.price, p.currency);

    nativeRequestPrice(id, currencyCode);
  }

  @SuppressWarnings("unused")
  private static void onPriceReceived(String id, String price, String currency)
  {
    if (TextUtils.isEmpty(price))
      return;

    sPriceCache.put(id, new Price(price, currency));

    OnPriceReceivedListener listener = sListener.get();
    if (listener == null)
      sListener = null;
    else
      listener.onPriceReceived(id, price, currency);
  }

  public static native SponsoredHotel nativeGetCurrent();
  private static native void nativeRequestPrice(String id, String currencyCode);
}
