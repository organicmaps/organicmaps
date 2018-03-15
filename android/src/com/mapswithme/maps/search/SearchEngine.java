package com.mapswithme.maps.search;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.util.Language;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.concurrency.UiThread;

import java.io.UnsupportedEncodingException;

public enum SearchEngine implements NativeSearchListener,
                                    NativeMapSearchListener
{
  INSTANCE;

  // Query, which results are shown on the map.
  @Nullable
  private String mQuery;

  @Override
  public void onResultsUpdate(final SearchResult[] results, final long timestamp,
                              final boolean isHotel)
  {
    UiThread.run(
        () ->
        {
          for (NativeSearchListener listener : mListeners)
            listener.onResultsUpdate(results, timestamp, isHotel);
          mListeners.finishIterate();
        });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(
        () ->
        {
          for (NativeSearchListener listener : mListeners)
            listener.onResultsEnd(timestamp);
          mListeners.finishIterate();
        });
  }

  @Override
  public void onMapSearchResults(final NativeMapSearchListener.Result[] results, final long timestamp, final boolean isLast)
  {
    UiThread.run(
        () ->
        {
          for (NativeMapSearchListener listener : mMapListeners)
            listener.onMapSearchResults(results, timestamp, isLast);
          mMapListeners.finishIterate();
        });
  }

  public void onBookmarksResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    // Dummy. Will be implemented soon.
  }

  public void onBookmarksResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    // Dummy. Will be implemented soon.
  }

  public void onFilterAvailableHotels(@Nullable FeatureId[] availableHotels)
  {
    UiThread.run(
        () ->
        {
          for (NativeBookingFilterListener listener : mHotelListeners)
            listener.onFilterAvailableHotels(availableHotels);
          mHotelListeners.finishIterate();
        });
  }

  @NonNull
  private final Listeners<NativeSearchListener> mListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeMapSearchListener> mMapListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeBookingFilterListener> mHotelListeners = new Listeners<>();

  public void addListener(NativeSearchListener listener)
  {
    mListeners.register(listener);
  }

  public void removeListener(NativeSearchListener listener)
  {
    mListeners.unregister(listener);
  }

  public void addMapListener(NativeMapSearchListener listener)
  {
    mMapListeners.register(listener);
  }

  public void removeMapListener(NativeMapSearchListener listener)
  {
    mMapListeners.unregister(listener);
  }

  public void addHotelListener(@NonNull NativeBookingFilterListener listener)
  {
    mHotelListeners.register(listener);
  }

  public void removeHotelListener(@NonNull NativeBookingFilterListener listener)
  {
    mHotelListeners.unregister(listener);
  }

  SearchEngine()
  {
    nativeInit();
  }

  private native void nativeInit();

  /**
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @return whether search was actually started.
   */
  @MainThread
  public boolean search(String query, long timestamp, boolean hasLocation,
                               double lat, double lon, @Nullable HotelsFilter hotelsFilter,
                               @Nullable BookingFilterParams bookingParams)
  {
    try
    {
      return nativeRunSearch(query.getBytes("utf-8"), Language.getKeyboardLocale(),
                             timestamp, hasLocation, lat, lon, hotelsFilter, bookingParams);
    } catch (UnsupportedEncodingException ignored) { }

    return false;
  }

  @MainThread
  public void searchInteractive(@NonNull String query, @NonNull String locale, long timestamp,
                                       boolean isMapAndTable, @Nullable HotelsFilter hotelsFilter,
                                       @Nullable BookingFilterParams bookingParams)
  {
    try
    {
      nativeRunInteractiveSearch(query.getBytes("utf-8"), locale, timestamp, isMapAndTable,
                                 hotelsFilter, bookingParams);
    } catch (UnsupportedEncodingException ignored) { }
  }

  @MainThread
  public void searchInteractive(@NonNull String query, long timestamp, boolean isMapAndTable,
                                       @Nullable HotelsFilter hotelsFilter, @Nullable BookingFilterParams bookingParams)
  {
    searchInteractive(query, Language.getKeyboardLocale(), timestamp, isMapAndTable, hotelsFilter, bookingParams);
  }

  @MainThread
  public static void searchMaps(String query, long timestamp)
  {
    try
    {
      nativeRunSearchMaps(query.getBytes("utf-8"), Language.getKeyboardLocale(), timestamp);
    } catch (UnsupportedEncodingException ignored) { }
  }

  public void setQuery(@Nullable String query)
  {
    mQuery = query;
  }

  @Nullable
  public String getQuery()
  {
    return mQuery;
  }

  @MainThread
  public void cancel()
  {
    cancelApiCall();
    cancelAllSearches();
  }
  @MainThread
  private static void cancelApiCall()
  {
    if (ParsedMwmRequest.hasRequest())
      ParsedMwmRequest.setCurrentRequest(null);
    Framework.nativeClearApiPoints();
  }

  @MainThread
  public void cancelInteractiveSearch()
  {
    mQuery = "";
    nativeCancelInteractiveSearch();
  }

  @MainThread
  private void cancelAllSearches()
  {
    mQuery = "";
    nativeCancelAllSearches();
  }

  @MainThread
  public void showResult(int index)
  {
    mQuery = "";
    nativeShowResult(index);
  }

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, String language, long timestamp, boolean hasLocation,
                                                double lat, double lon, @Nullable HotelsFilter hotelsFilter,
                                                @Nullable BookingFilterParams bookingParams);

  /**
   * @param bytes utf-8 formatted query bytes
   * @param bookingParams
   */
  private static native void nativeRunInteractiveSearch(byte[] bytes, String language, long timestamp,
                                                        boolean isMapAndTable, @Nullable HotelsFilter hotelsFilter,
                                                        @Nullable BookingFilterParams bookingParams);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunSearchMaps(byte[] bytes, String language, long timestamp);

  private static native void nativeShowResult(int index);

  private static native void nativeCancelInteractiveSearch();

  private static native void nativeCancelEverywhereSearch();

  private static native void nativeCancelAllSearches();

  /**
   * @return all existing hotel types
   */
  @NonNull
  static native HotelsFilter.HotelType[] nativeGetHotelTypes();
}
