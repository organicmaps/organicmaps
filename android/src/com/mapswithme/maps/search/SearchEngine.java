package com.mapswithme.maps.search;

import android.content.Context;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.util.Language;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.concurrency.UiThread;

import java.nio.charset.StandardCharsets;

public enum SearchEngine implements NativeSearchListener,
                                    NativeMapSearchListener,
                                    NativeBookmarkSearchListener,
                                    Initializable<Void>
{
  INSTANCE;

  // Query, which results are shown on the map.
  @Nullable
  private String mQuery;

  @Override
  public void onResultsUpdate(@NonNull final SearchResult[] results, final long timestamp)
  {
    UiThread.run(
        () ->
        {
          for (NativeSearchListener listener : mListeners)
            listener.onResultsUpdate(results, timestamp);
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

  public void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (NativeBookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsUpdate(bookmarkIds, timestamp);
    mBookmarkListeners.finishIterate();
  }

  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (NativeBookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsEnd(bookmarkIds, timestamp);
    mBookmarkListeners.finishIterate();
  }

  @NonNull
  private final Listeners<NativeSearchListener> mListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeMapSearchListener> mMapListeners = new Listeners<>();
  @NonNull
  private final Listeners<NativeBookmarkSearchListener> mBookmarkListeners = new Listeners<>();

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

  public void addBookmarkListener(NativeBookmarkSearchListener listener)
  {
    mBookmarkListeners.register(listener);
  }

  public void removeBookmarkListener(NativeBookmarkSearchListener listener)
  {
    mBookmarkListeners.unregister(listener);
  }

  private native void nativeInit();

  /**
   *
   * @param context
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @return whether search was actually started.
   */
  @MainThread
  public boolean search(@NonNull Context context, String query, boolean isCategory,
                        long timestamp, boolean hasLocation, double lat, double lon)
  {
    return nativeRunSearch(query.getBytes(StandardCharsets.UTF_8), isCategory,
            Language.getKeyboardLocale(context), timestamp, hasLocation, lat, lon);
  }

  @MainThread
  public void searchInteractive(@NonNull String query, boolean isCategory, @NonNull String locale,
                                long timestamp, boolean isMapAndTable)
  {
    nativeRunInteractiveSearch(query.getBytes(StandardCharsets.UTF_8), isCategory,
            locale, timestamp, isMapAndTable);
  }

  @MainThread
  public void searchInteractive(@NonNull Context context, @NonNull String query, boolean isCategory,
                                long timestamp, boolean isMapAndTable)
  {
    searchInteractive(query, isCategory, Language.getKeyboardLocale(context), timestamp, isMapAndTable);
  }

  @MainThread
  public static void searchMaps(@NonNull Context context, String query, long timestamp)
  {
    nativeRunSearchMaps(query.getBytes(StandardCharsets.UTF_8), Language.getKeyboardLocale(context),
                        timestamp);
  }

  @MainThread
  public boolean searchInBookmarks(@NonNull String query, long categoryId, long timestamp)
  {
    return nativeRunSearchInBookmarks(query.getBytes(StandardCharsets.UTF_8), categoryId, timestamp);
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
    if (ParsedMwmRequest.getCurrentRequest() != null)
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

  @Override
  public void initialize(@Nullable Void aVoid)
  {
    nativeInit();
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, boolean isCategory,
                                                String language, long timestamp, boolean hasLocation,
                                                double lat, double lon);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunInteractiveSearch(byte[] bytes, boolean isCategory,
                                                        String language, long timestamp,
                                                        boolean isMapAndTable);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunSearchMaps(byte[] bytes, String language, long timestamp);

  private static native boolean nativeRunSearchInBookmarks(byte[] bytes, long categoryId, long timestamp);

  private static native void nativeShowResult(int index);

  private static native void nativeCancelInteractiveSearch();

  private static native void nativeCancelEverywhereSearch();

  private static native void nativeCancelAllSearches();
}
