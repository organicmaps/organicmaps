package com.mapswithme.maps.search;

import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Pair;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.Language;
import com.mapswithme.util.concurrency.UiThread;

import java.util.ArrayList;
import java.util.List;

public enum SearchEngine implements NativeSearchListener
{
  INSTANCE;

  private final List<String> mRecentQueries = new ArrayList<>();

  public void refreshRecents()
  {
    List<Pair<String, String>> pairs = new ArrayList<>();
    Framework.nativeGetRecentSearchQueries(pairs);
    mRecentQueries.clear();

    for (Pair<String, String> pair : pairs)
      mRecentQueries.add(pair.second);
  }

  public int getRecentsSize()
  {
    return mRecentQueries.size();
  }

  public String getRecent(int position)
  {
    return mRecentQueries.get(position);
  }

  public boolean addRecent(@NonNull String query)
  {
    query = query.trim();
    if (TextUtils.isEmpty(query) || mRecentQueries.contains(query))
      return false;

    Framework.nativeAddRecentSearchQuery(Language.getKeyboardLocale(), query);
    refreshRecents();
    return true;
  }

  public void clearRecents()
  {
    Framework.nativeClearRecentSearchQueries();
    mRecentQueries.clear();
  }

  @Override
  public void onResultsUpdate(final int count, final long timestamp)
  {
    UiThread.run(new Runnable() {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsUpdate(count, timestamp);
      }
    });
  }

  @Override
  public void onResultsEnd(final long timestamp)
  {
    UiThread.run(new Runnable() {
      @Override
      public void run()
      {
        for (NativeSearchListener listener : mListeners)
          listener.onResultsEnd(timestamp);
      }
    });
  }

  private List<NativeSearchListener> mListeners = new ArrayList<>();

  public void addListener(NativeSearchListener listener)
  {
    mListeners.add(listener);
  }

  public boolean removeListener(NativeSearchListener listener)
  {
    return mListeners.remove(listener);
  }

  SearchEngine()
  {
    nativeInit();
  }

  private native void nativeInit();

  public static native void nativeShowResult(int position);

  public static native void nativeShowAllResults();

  public static native SearchResult nativeGetResult(int position, long timestamp, boolean hasLocation, double lat, double lon);

  /**
   * @param timestamp Search results are filtered according to it after multiple requests.
   * @param force Should be false for repeating requests with the same query.
   * @return whether search was actually started.
   */
  public static native boolean nativeRunSearch(String query, String language, long timestamp, boolean force, boolean hasLocation, double lat, double lon);

  public static native void nativeRunInteractiveSearch(String query, String language, long timestamp);
}
