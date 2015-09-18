package com.mapswithme.maps.search;

import android.util.SparseArray;

/**
 * Helper to avoid unnecessary invocations of native functions to obtain search result objects.
 */
class CachedResults
{
  private final SearchFragment mFragment;
  private final SparseArray<SearchResult> mCache = new SparseArray<>();
  private int mCurrentQueryId = -1;

  public CachedResults(SearchFragment fragment)
  {
    mFragment = fragment;
  }

  public SearchResult get(int position)
  {
    SearchResult res = mCache.get(position);

    if (res == null)
    {
      res = mFragment.getUncachedResult(position);
      mCache.put(position, res);
    }
    return res;
  }

  public void clear()
  {
    mCache.clear();
  }
}
