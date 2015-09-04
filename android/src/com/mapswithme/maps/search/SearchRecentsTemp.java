package com.mapswithme.maps.search;

import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Pair;
import com.mapswithme.maps.Framework;
import com.mapswithme.util.Language;

import java.util.ArrayList;
import java.util.List;

// KILLME: Temp class to be moved into SearchEngine
enum SearchRecentsTemp
{
  INSTANCE;

  private final List<String> mQueries = new ArrayList<>();

  public void refresh()
  {
    List<Pair<String, String>> pairs = new ArrayList<>();
    Framework.nativeGetRecentSearchQueries(pairs);
    mQueries.clear();

    for (Pair<String, String> pair : pairs)
      mQueries.add(pair.second);
  }

  public int getSize()
  {
    return mQueries.size();
  }

  public String get(int position)
  {
    return mQueries.get(position);
  }

  public boolean add(@NonNull String query)
  {
    query = query.trim();
    if (TextUtils.isEmpty(query) || mQueries.contains(query))
      return false;

    Framework.nativeAddRecentSearchQuery(Language.getKeyboardLocale(), query);
    refresh();
    return true;
  }

  public void clear()
  {
    Framework.nativeClearRecentSearchQueries();
    mQueries.clear();
  }
}
