package com.mapswithme.maps.search;

import java.io.UnsupportedEncodingException;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.util.Language;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.concurrency.UiThread;

public enum SearchEngine implements NativeSearchListener,
                                    NativeMapSearchListener
{
  INSTANCE;

  // Query, which results are shown on the map.
  private static String sSavedQuery;

  @Override
  public void onResultsUpdate(final SearchResult[] results, final long timestamp)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsUpdate(results, timestamp);
        mListeners.finishIterate();
      }
    });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsEnd(timestamp);
        mListeners.finishIterate();
      }
    });
  }

  @Override
  public void onMapSearchResults(final NativeMapSearchListener.Result[] results, final long timestamp, final boolean isLast)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        for (NativeMapSearchListener listener : mMapListeners)
          listener.onMapSearchResults(results, timestamp, isLast);
        mMapListeners.finishIterate();
      }
    });
  }

  private final Listeners<NativeSearchListener> mListeners = new Listeners<>();
  private final Listeners<NativeMapSearchListener> mMapListeners = new Listeners<>();

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

  SearchEngine()
  {
    nativeInit();
  }

  private native void nativeInit();

  /**
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @param force     Should be false for repeating requests with the same query.
   * @return whether search was actually started.
   */
  public static boolean search(String query, long timestamp, boolean force, boolean hasLocation, double lat, double lon)
  {
    try
    {
      return nativeRunSearch(query.getBytes("utf-8"), Language.getKeyboardLocale(), timestamp, force, hasLocation, lat, lon);
    } catch (UnsupportedEncodingException ignored) { }

    return false;
  }

  public static void searchInteractive(String query, long timestamp, boolean isMapAndTable)
  {
    try
    {
      nativeRunInteractiveSearch(query.getBytes("utf-8"), Language.getKeyboardLocale(), timestamp, isMapAndTable);
    } catch (UnsupportedEncodingException ignored) { }
  }

  public static void searchMaps(String query, long timestamp)
  {
    try
    {
      nativeRunSearchMaps(query.getBytes("utf-8"), Language.getKeyboardLocale(), timestamp);
    } catch (UnsupportedEncodingException ignored) { }
  }

  public static String getQuery()
  {
    return sSavedQuery;
  }

  public static void cancelApiCall()
  {
    if (ParsedMwmRequest.hasRequest())
      ParsedMwmRequest.setCurrentRequest(null);
    Framework.nativeClearApiPoints();
  }

  public static void cancelSearch()
  {
    sSavedQuery = "";
    nativeCancelInteractiveSearch();
  }

  public static void showResult(int index)
  {
    sSavedQuery = "";
    nativeShowResult(index);
  }

  public static void showAllResults(String query)
  {
    sSavedQuery = query;
    nativeShowAllResults();
  }

  /**
   * @param bytes utf-8 formatted bytes of query.
   */
  private static native boolean nativeRunSearch(byte[] bytes, String language, long timestamp, boolean force, boolean hasLocation, double lat, double lon);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunInteractiveSearch(byte[] bytes, String language, long timestamp, boolean isMapAndTable);

  /**
   * @param bytes utf-8 formatted query bytes
   */
  private static native void nativeRunSearchMaps(byte[] bytes, String language, long timestamp);

  private static native void nativeShowResult(int index);

  private static native void nativeShowAllResults();

  public static native void nativeCancelInteractiveSearch();
}
