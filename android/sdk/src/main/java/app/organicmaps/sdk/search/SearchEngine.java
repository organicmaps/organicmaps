package app.organicmaps.sdk.search;

import android.content.Context;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.nio.charset.StandardCharsets;
import org.chromium.base.ObserverList;

/**
 * Process-wide asynchronous search over installed maps and bookmark collections.
 * Use on the main thread after {@link app.organicmaps.sdk.OrganicMaps#init} completes map loading.
 * Register listeners before starting a request and remove them when the host ends. Supply a new monotonically
 * increasing request timestamp and compare it in callbacks to ignore results from older requests. Search
 * results are successive complete snapshots, not batches to append. Detailed offline results require region maps.
 */
public enum SearchEngine implements SearchListener, MapSearchListener,
                                    BookmarkSearchListener
{
  INSTANCE;

  // Query, which results are shown on the map.
  @Nullable
  private String mQuery;

  @Nullable
  private SearchResult[] mCachedResults = null;
  @Nullable
  private String mCachedSearchBarQuery = null;

  @Override
  public void onResultsUpdate(@NonNull final SearchResult[] results, final long timestamp)
  {
    UiThread.run(() -> {
      mCachedResults = results;
      for (SearchListener listener : mListeners)
        listener.onResultsUpdate(results, timestamp);
    });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(() -> {
      for (SearchListener listener : mListeners)
        listener.onResultsEnd(timestamp);
    });
  }

  @Override
  public void onMapSearchResults(@NonNull final MapSearchListener.Result[] results, final long timestamp,
                                 final boolean isLast)
  {
    UiThread.run(() -> {
      for (MapSearchListener listener : mMapListeners)
        listener.onMapSearchResults(results, timestamp, isLast);
    });
  }

  @Override
  public void onBookmarkSearchResultsUpdate(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (BookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsUpdate(bookmarkIds, timestamp);
  }

  @Override
  public void onBookmarkSearchResultsEnd(@Nullable long[] bookmarkIds, long timestamp)
  {
    for (BookmarkSearchListener listener : mBookmarkListeners)
      listener.onBookmarkSearchResultsEnd(bookmarkIds, timestamp);
  }

  private final ObserverList<SearchListener> mListeners = new ObserverList<>();

  private final ObserverList<MapSearchListener> mMapListeners = new ObserverList<>();

  private final ObserverList<BookmarkSearchListener> mBookmarkListeners = new ObserverList<>();

  /**
   * Registers an ordinary search listener; registration does not replay cached results.
   */
  public void addListener(SearchListener listener)
  {
    mListeners.addObserver(listener);
  }

  /**
   * Removes an ordinary search listener. Already queued callbacks should still be checked against request state.
   */
  public void removeListener(SearchListener listener)
  {
    mListeners.removeObserver(listener);
  }

  public void addMapListener(MapSearchListener listener)
  {
    mMapListeners.addObserver(listener);
  }

  public void removeMapListener(MapSearchListener listener)
  {
    mMapListeners.removeObserver(listener);
  }

  public void addBookmarkListener(BookmarkSearchListener listener)
  {
    mBookmarkListeners.addObserver(listener);
  }

  public void removeBookmarkListener(BookmarkSearchListener listener)
  {
    mBookmarkListeners.removeObserver(listener);
  }

  /**
   * Starts an ordinary search using the current keyboard locale. Results arrive through {@link SearchListener}.
   *
   * @param context context used to choose the query locale
   * @param query text to search for
   * @param isCategory whether the query is a category rather than free text
   * @param timestamp monotonically increasing request identifier echoed by callbacks
   * @param hasLocation whether the following coordinates are available for ranking
   * @param lat WGS84 latitude in degrees, ignored when {@code hasLocation} is false
   * @param lon WGS84 longitude in degrees, ignored when {@code hasLocation} is false
   * @return whether the native engine started a search
   */
  @MainThread
  public boolean search(@NonNull Context context, @NonNull String query, boolean isCategory, long timestamp,
                        boolean hasLocation, double lat, double lon)
  {
    boolean started = nativeRunSearch(query.getBytes(StandardCharsets.UTF_8), isCategory,
                                      Language.getKeyboardLocale(context), timestamp, hasLocation, lat, lon);
    if (started)
      mCachedSearchBarQuery = query;
    return started;
  }

  /**
   * Starts a search that updates with the map viewport.
   *
   * @param query search text
   * @param isCategory whether this is a category query
   * @param locale query language code
   * @param timestamp monotonically increasing request identifier echoed by callbacks
   * @param isMapAndTable whether to deliver list results as well as map marks; false searches the viewport only
   * @param hasLocation whether a current location is available for ranking
   * @param lat current WGS84 latitude in degrees
   * @param lon current WGS84 longitude in degrees
   * @return whether the native engine started a search
   */
  @MainThread
  public boolean searchInteractive(@NonNull String query, boolean isCategory, @NonNull String locale, long timestamp,
                                   boolean isMapAndTable, boolean hasLocation, double lat, double lon)
  {
    final boolean started = nativeRunInteractiveSearch(query.getBytes(StandardCharsets.UTF_8), isCategory, locale,
                                                       timestamp, isMapAndTable, hasLocation, lat, lon);
    // Cache the search-bar query only for map+table searches. Viewport-only searches (e.g. the
    // navigation search wheel) don't deliver list results, so caching their query would pair it
    // with the previous search's cached results when the search fragment is recreated.
    if (started && isMapAndTable)
      mCachedSearchBarQuery = query;
    return started;
  }

  /**
   * Starts interactive search with an explicit locale and without a current-location ranking hint.
   * @see #searchInteractive(String, boolean, String, long, boolean, boolean, double, double)
   */
  @MainThread
  public boolean searchInteractive(@NonNull String query, boolean isCategory, @NonNull String locale, long timestamp,
                                   boolean isMapAndTable)
  {
    return searchInteractive(query, isCategory, locale, timestamp, isMapAndTable, false, 0, 0);
  }

  /**
   * Starts interactive search using the keyboard locale and no current-location ranking hint.
   * @see #searchInteractive(String, boolean, String, long, boolean, boolean, double, double)
   */
  @MainThread
  public boolean searchInteractive(@NonNull Context context, @NonNull String query, boolean isCategory, long timestamp,
                                   boolean isMapAndTable)
  {
    return searchInteractive(query, isCategory, Language.getKeyboardLocale(context), timestamp, isMapAndTable, false, 0,
                             0);
  }

  /**
   * Searches downloadable region names; results arrive through {@link MapSearchListener}.
   * @param context context used to select the query locale
   * @param query region-name query
   * @param timestamp request identifier echoed by callbacks
   */
  @MainThread
  public static void searchMaps(@NonNull Context context, @NonNull String query, long timestamp)
  {
    nativeRunSearchMaps(query.getBytes(StandardCharsets.UTF_8), Language.getKeyboardLocale(context), timestamp);
  }

  /**
   * Searches bookmarks in a category; results arrive through {@link BookmarkSearchListener}.
   * @param query search text
   * @param categoryId native bookmark-category identifier
   * @param timestamp request identifier echoed by callbacks
   * @return whether the native engine started a search
   */
  @MainThread
  public boolean searchInBookmarks(@NonNull String query, long categoryId, long timestamp)
  {
    return nativeRunSearchInBookmarks(query.getBytes(StandardCharsets.UTF_8), categoryId, timestamp);
  }

  /**
   * Stores the query associated with map search presentation; this does not start a search.
   */
  public void setQuery(@Nullable String query)
  {
    mQuery = query;
  }

  /**
   * Returns the query stored for map presentation, or {@code null} if none was set.
   */
  @Nullable
  public String getQuery()
  {
    return mQuery;
  }

  /**
   * Cancels active searches, clears cached query/results, and clears map API points.
   * Callbacks already queued may still arrive; invalidate the host's request identifier too.
   */
  @MainThread
  public void cancel()
  {
    cancelApiCall();
    cancelAllSearches();
  }

  @MainThread
  private static void cancelApiCall()
  {
    Framework.nativeClearApiPoints();
  }

  /**
   * Cancels viewport search and clears its cached query/results. Already queued callbacks may still arrive.
   */
  @MainThread
  public void cancelInteractiveSearch()
  {
    mQuery = "";
    mCachedResults = null;
    mCachedSearchBarQuery = null;
    nativeCancelInteractiveSearch();
  }

  @MainThread
  private void cancelAllSearches()
  {
    mQuery = "";
    mCachedResults = null;
    mCachedSearchBarQuery = null;
    nativeCancelAllSearches();
  }

  /**
   * Selects a result from the latest native result snapshot, adjusts the viewport, and clears the stored query.
   * @param index index in the latest result snapshot, not an index in a filtered display list
   */
  @MainThread
  public void showResult(int index)
  {
    mQuery = "";
    nativeShowResult(index);
  }

  /**
   * Selects a result while preserving interactive-search highlighting.
   * @param index index in the latest native result snapshot
   */
  @MainThread
  public void selectResult(int index)
  {
    // Do not clear mQuery here to preserve interactive search highlights on the map.
    nativeSelectResult(index);
  }

  /**
   * Fits the viewport to the last search results according to the native search viewport policy.
   */
  @MainThread
  public void updateViewportWithLastResults()
  {
    nativeUpdateViewportWithLastResults();
  }

  /**
   * Returns the last delivered ordinary result snapshot, or {@code null} after cancellation or before results.
   * Treat the returned array as read-only.
   */
  @Nullable
  public SearchResult[] getCachedResults()
  {
    return mCachedResults;
  }

  /**
   * Returns the query associated with cached list results, or {@code null} when none is cached.
   */
  @Nullable
  public String getCachedSearchBarQuery()
  {
    return mCachedSearchBarQuery;
  }

  /**
   * Installs native search callbacks. Called by {@link app.organicmaps.sdk.OrganicMaps#init}; hosts should not repeat
   * it.
   */
  public void initialize()
  {
    nativeInit();
  }

  private native void nativeInit();

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, boolean isCategory, String language, long timestamp,
                                                boolean hasLocation, double lat, double lon);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native boolean nativeRunInteractiveSearch(byte[] bytes, boolean isCategory, String language,
                                                           long timestamp, boolean isMapAndTable, boolean hasLocation,
                                                           double lat, double lon);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunSearchMaps(byte[] bytes, String language, long timestamp);

  private static native boolean nativeRunSearchInBookmarks(byte[] bytes, long categoryId, long timestamp);

  private static native void nativeShowResult(int index);

  private static native void nativeSelectResult(int index);

  private static native void nativeUpdateViewportWithLastResults();

  private static native void nativeCancelInteractiveSearch();

  private static native void nativeCancelAllSearches();
}
